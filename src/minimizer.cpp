#include "minimizer.hpp"

#include "evaluator.hpp"
#include "optimizer.hpp"
#include "program_util.hpp"
#include "semantics.hpp"
#include "util.hpp"

#include <fstream>

bool Minimizer::minimize( Program &p, size_t num_terms ) const
{
  Log::get().debug( "Minimizing program" );
  Evaluator evaluator( settings );

  // calculate target sequence
  Sequence target_sequence;
  steps_t target_steps = evaluator.eval( p, target_sequence, num_terms, false );
  if ( target_sequence.size() < settings.num_terms )
  {
    Log::get().error(
        "Cannot minimize program because there are too few terms: " + std::to_string( target_sequence.size() ), false );
    return false;
  }

  // remove "clr" operations
  bool global_change = removeClr( p );

  // remove or replace operations
  bool local_change;
  for ( int64_t i = 0; i < (int64_t) p.ops.size(); ++i )
  {
    local_change = false;
    const auto op = p.ops[i]; // make a backup of the original operation
    if ( op.type == Operation::Type::LPE )
    {
      continue;
    }
    else if ( op.type == Operation::Type::TRN )
    {
      p.ops[i].type = Operation::Type::SUB;
      bool can_replace;
      try
      {
        auto res = evaluator.check( p, target_sequence );
        can_replace = (res.first == status_t::OK) && (res.second.total <= target_steps.total);
      }
      catch ( const std::exception& )
      {
        can_replace = false;
      }
      if ( can_replace )
      {
        local_change = true;
      }
      else
      {
        // revert change
        p.ops[i] = op;
      }
    }
    else if ( op.type == Operation::Type::LPB )
    {
      if ( op.source.type != Operand::Type::CONSTANT || op.source.value != 1 )
      {
        p.ops[i].source = Operand( Operand::Type::CONSTANT, 1 );
        bool can_reset;
        try
        {
          auto res = evaluator.check( p, target_sequence );
          can_reset = (res.first == status_t::OK) && (res.second.total <= target_steps.total);
        }
        catch ( const std::exception& )
        {
          can_reset = false;
        }
        if ( can_reset )
        {
          local_change = true;
        }
        else
        {
          // revert change
          p.ops[i] = op;
        }
      }
    }
    else if ( p.ops.size() > 1 ) // try to remove the current operation (if there is at least one operation, see A000004)
    {
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 1 );
      bool can_remove;
      try
      {
        auto res = evaluator.check( p, target_sequence );
        can_remove = (res.first == status_t::OK) && (res.second.total <= target_steps.total);
      }
      catch ( const std::exception& )
      {
        can_remove = false;
      }
      if ( can_remove )
      {
        local_change = true;
        --i;
      }
      else
      {
        // revert change
        p.ops.insert( p.ops.begin() + i, op );
      }
    }

    if ( !local_change )
    {
      // gcd with larger power of small constant? => replace with a loop
      if ( op.type == Operation::Type::GCD && op.target.type == Operand::Type::DIRECT
          && op.source.type == Operand::Type::CONSTANT )
      {
        int64_t base = 0;
        int64_t v = op.source.value.asInt();
        if ( getPowerOf( v, 2 ) >= 10 )
        {
          base = 2;
        }
        else if ( getPowerOf( v, 3 ) >= 6 )
        {
          base = 3;
        }
        else if ( getPowerOf( v, 5 ) >= 5 )
        {
          base = 5;
        }
        else if ( getPowerOf( v, 7 ) >= 4 )
        {
          base = 7;
        }
        else if ( getPowerOf( v, 10 ) >= 3 )
        {
          base = 10;
        }
        if ( base != 0 )
        {
          std::unordered_set<int64_t> used_cells;
          int64_t largest_used = 0;
          if ( ProgramUtil::getUsedMemoryCells( p, used_cells, largest_used, settings.max_memory ) )
          {
            // try to replace gcd by a loop
            auto tmp = Operand( Operand::Type::DIRECT, largest_used + 1 );
            p.ops[i] = Operation( Operation::Type::MOV, tmp, Operand( Operand::Type::CONSTANT, 1 ) );
            p.ops.insert( p.ops.begin() + i + 1,
                Operation( Operation::Type::LPB, op.target, Operand( Operand::Type::CONSTANT, 1 ) ) );
            p.ops.insert( p.ops.begin() + i + 2,
                Operation( Operation::Type::MUL, tmp, Operand( Operand::Type::CONSTANT, base ) ) );
            p.ops.insert( p.ops.begin() + i + 3,
                Operation( Operation::Type::DIF, op.target, Operand( Operand::Type::CONSTANT, base ) ) );
            p.ops.insert( p.ops.begin() + i + 4, Operation( Operation::Type::LPE ) );
            p.ops.insert( p.ops.begin() + i + 5, Operation( Operation::Type::MOV, op.target, tmp ) );

            bool can_rewrite;
            try
            {
              auto res = evaluator.check( p, target_sequence );
              can_rewrite = (res.first == status_t::OK); // we don't check the number of steps here!
            }
            catch ( const std::exception& )
            {
              can_rewrite = false;
            }
            if ( can_rewrite )
            {
              local_change = true;
            }
            else
            {
              // revert change
              p.ops[i] = op;
              p.ops.erase( p.ops.begin() + i + 1, p.ops.begin() + i + 6 );
            }
          }
        }
      }
    }
    global_change = global_change || local_change;
  }
  return global_change;
}

bool Minimizer::removeClr( Program &p ) const
{
  bool replaced = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    if ( p.ops[i].type == Operation::Type::CLR && p.ops[i].source.type == Operand::Type::CONSTANT )
    {
      const int64_t length = p.ops[i].source.value.asInt();
      if ( length <= 0 )
      {
        p.ops.erase( p.ops.begin() + i );
      }
      else
      {
        p.ops[i].type = Operation::Type::MOV;
        p.ops[i].source.value = 0;
        auto mov = p.ops[i];
        for ( int64_t j = 1; j < length; j++ )
        {
          mov.target.value = Semantics::add( mov.target.value, Number::ONE );
          p.ops.insert( p.ops.begin() + i + j, mov );
        }
      }
      replaced = true;
    }
  }
  return replaced;
}

bool Minimizer::optimizeAndMinimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells,
    size_t num_terms ) const
{
  Program backup = p;
  try
  {
    bool optimized = false, minimized = false, result = false;
    do
    {
      optimized = optimizer.optimize( p, num_reserved_cells, num_initialized_cells );
      minimized = minimize( p, num_terms );
      result = result || optimized || minimized;
    } while ( optimized || minimized );
    return result;
  }
  catch ( std::exception& e )
  {
    // revert change
    p = backup;
    // log error and dump program for later analysis
    Log::get().error( "Exception during minimization: " + std::string( e.what() ), false );
    std::string f = getLodaHome() + "debug/minimizer/" + std::to_string( ProgramUtil::hash( p ) % 100000 ) + ".asm";
    ensureDir( f );
    std::ofstream out( f );
    ProgramUtil::print( p, out );
  }
  return false;
}
