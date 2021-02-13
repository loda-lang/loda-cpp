#include "minimizer.hpp"

#include "interpreter.hpp"
#include "optimizer.hpp"
#include "util.hpp"

void Minimizer::minimize( Program &p, size_t num_terms ) const
{
  Log::get().debug( "Minimizing program" );
  Interpreter interpreter( settings );
  Sequence target_sequence;
  try
  {
    interpreter.eval( p, target_sequence, num_terms );
  }
  catch ( const std::exception& )
  {
    return;
  }
  int removed_ops = 0;
  for ( int i = 0; i < (int) p.ops.size(); ++i )
  {
    auto op = p.ops.at( i );
    if ( op.type == Operation::Type::LPE )
    {
      continue;
    }
    else if ( op.type == Operation::Type::TRN )
    {
      p.ops.at( i ).type = Operation::Type::SUB;
      bool can_replace;
      try
      {
        can_replace = interpreter.check( p, target_sequence );
      }
      catch ( const std::exception& )
      {
        can_replace = false;
      }
      if ( !can_replace )
      {
        p.ops.at( i ) = op;
      }
    }
    else if ( op.type == Operation::Type::LPB )
    {
      if ( op.source.type != Operand::Type::CONSTANT || op.source.value != 1 )
      {
        p.ops.at( i ).source = Operand( Operand::Type::CONSTANT, 1 );
        bool can_reset;
        try
        {
          can_reset = interpreter.check( p, target_sequence );
        }
        catch ( const std::exception& )
        {
          can_reset = false;
        }
        if ( !can_reset )
        {
          p.ops.at( i ) = op;
        }
      }
    }
    else
    {
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 1 );
      bool can_remove;
      try
      {
        can_remove = interpreter.check( p, target_sequence );
      }
      catch ( const std::exception& )
      {
        can_remove = false;
      }
      if ( !can_remove )
      {
        p.ops.insert( p.ops.begin() + i, op );
      }
      else
      {
        --i;
        ++removed_ops;
      }
    }
  }
  // avoid empty program for zero sequence (A000004)
  if ( p.ops.empty() )
  {
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
        Program::INPUT_CELL );
  }
  if ( removed_ops > 0 )
  {
    Log::get().debug( "Removed " + std::to_string( removed_ops ) + " operations during minimization" );
  }
}

void Minimizer::optimizeAndMinimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells,
    size_t num_terms ) const
{
  Optimizer optimizer( settings );
  size_t length;
  do
  {
    length = p.ops.size();
    optimizer.optimize( p, num_reserved_cells, num_initialized_cells );
    minimize( p, num_terms );
  } while ( p.ops.size() < length );
}
