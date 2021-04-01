#include "minimizer.hpp"

#include "interpreter.hpp"
#include "optimizer.hpp"
#include "program_util.hpp"
#include "util.hpp"

bool Minimizer::minimize( Program &p, size_t num_terms ) const
{
  Log::get().debug( "Minimizing program" );
  Interpreter interpreter( settings );

  // calculate target sequence
  Sequence target_sequence;
  steps_t target_steps;
  try
  {
    target_steps = interpreter.eval( p, target_sequence, num_terms );
  }
  catch ( const std::exception& )
  {
    return false;
  }

  // remove or replace operations
  bool global_change = false, local_change;
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
        auto res = interpreter.check( p, target_sequence );
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
          auto res = interpreter.check( p, target_sequence );
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
        auto res = interpreter.check( p, target_sequence );
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
        number_t base = 0;
        number_t v = op.source.value;
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
          std::unordered_set<number_t> used_cells;
          number_t largest_used = 0;
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
              auto res = interpreter.check( p, target_sequence );
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

bool Minimizer::optimizeAndMinimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells,
    size_t num_terms ) const
{
  Optimizer optimizer( settings );
  bool optimized = false, minimized = false, result = false;
  do
  {
    optimized = optimizer.optimize( p, num_reserved_cells, num_initialized_cells );
    minimized = minimize( p, num_terms );
    result = result || optimized || minimized;
  } while ( optimized || minimized );
  return result;
}
