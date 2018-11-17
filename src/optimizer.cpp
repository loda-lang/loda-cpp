#include "optimizer.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "util.hpp"

#include <unordered_set>

void Optimizer::optimize( Program& p, size_t num_initialized_cells )
{
  removeNops( p );
  while ( removeEmptyLoops( p ) )
    ;
  simplifyOperands( p, num_initialized_cells );
  mergeOps( p );
}

bool Optimizer::removeNops( Program& p )
{
  bool removed = false;
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == Operation::Type::NOP || it->type == Operation::Type::DBG
        || ((it->type == Operation::Type::ADD || it->type == Operation::Type::SUB)
            && it->source.type == Operand::Type::CONSTANT && it->source.value == 0)
        || (it->type == Operation::Type::MOV && it->source == it->target) )
    {
      it = p.ops.erase( it );
      removed = true;
    }
    else
    {
      ++it;
    }
  }
  return removed;
}

bool Optimizer::removeEmptyLoops( Program& p )
{
  bool removed = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    if ( i + 1 < p.ops.size() && p.ops[i].type == Operation::Type::LPB && p.ops[i + 1].type == Operation::Type::LPE )
    {
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Removing empty loop" );
      }
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 2 );
      i = i - 2;
      removed = true;
    }
  }
  return removed;
}

bool Optimizer::mergeOps( Program& p )
{
  bool merged = false;
  for ( size_t i = 0; i + 1 < p.ops.size(); i++ )
  {
    bool do_merge = false;

    auto& o1 = p.ops[i];
    auto& o2 = p.ops[i + 1];

    // operation targets the same?
    if ( o1.target == o2.target )
    {
      // operation types the same?
      if ( o1.type == o2.type )
      {
        // both sources constants?
        if ( o1.source.type == Operand::Type::CONSTANT && o2.source.type == Operand::Type::CONSTANT )
        {
          // both add or sub operation?
          if ( o1.type == Operation::Type::ADD || o1.type == Operation::Type::SUB )
          {
            o1.source.value += o2.source.value;
            do_merge = true;
          }
        }
      }

      // second operation mov?
      if ( o2.type == Operation::Type::MOV )
      {
        // first operation add, sub, mov?
        if ( o1.type == Operation::Type::ADD || o1.type == Operation::Type::SUB || o1.type == Operation::Type::MOV )
        {
          // second mov overwrites first operation
          o1 = o2;
          do_merge = true;
        }
      }
    }

    // merge (erase second operation)
    if ( do_merge )
    {
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Merging similar consecutive operation" );
      }
      p.ops.erase( p.ops.begin() + i + 1, p.ops.begin() + i + 2 );
      --i;
      merged = true;
    }
  }

  return merged;
}

inline void simplifyOperand( Operand& op, std::unordered_set<number_t>& initialized_cells, bool is_source )
{
  switch ( op.type )
  {
  case Operand::Type::CONSTANT:
    break;
  case Operand::Type::MEM_ACCESS_DIRECT:
    if ( initialized_cells.find( op.value ) == initialized_cells.end() && is_source )
    {
      op.type = Operand::Type::CONSTANT;
      op.value = 0;
    }
    break;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    if ( initialized_cells.find( op.value ) == initialized_cells.end() )
    {
      op.type = Operand::Type::MEM_ACCESS_DIRECT;
      op.value = 0;
    }
    break;
  }
}

void Optimizer::simplifyOperands( Program& p, size_t num_initialized_cells )
{
  std::unordered_set<number_t> initialized_cells;
  for ( number_t i = 0; i < num_initialized_cells; ++i )
  {
    initialized_cells.insert( i );
  }
  for ( auto& op : p.ops )
  {
    switch ( op.type )
    {
    case Operation::Type::ADD:
    case Operation::Type::SUB:
    case Operation::Type::MOV:
    {
      simplifyOperand( op.source, initialized_cells, true );
      simplifyOperand( op.target, initialized_cells, false );
      switch ( op.target.type )
      {
      case Operand::Type::MEM_ACCESS_DIRECT:
        initialized_cells.insert( op.target.value );
        break;
      case Operand::Type::MEM_ACCESS_INDIRECT:
        return; // don't know at this point which cell is written to
      case Operand::Type::CONSTANT:
        Log::get().error( "invalid program" );
        break;
      }
      break;
    }

    case Operation::Type::NOP:
    case Operation::Type::DBG:
      break;

    default:
      return; // not know what happens in other cases

    }
  }
}

void Optimizer::minimize( Program& p, size_t num_terms )
{
  Settings settings;
  settings.num_terms = num_terms;
  Interpreter interpreter( settings );
  Sequence target_sequence;
  try
  {
    target_sequence = interpreter.eval( p );
  }
  catch ( const std::exception& )
  {
    return;
  }
  int removed_ops = 0;
  for ( int i = 0; i < (int) p.ops.size(); ++i )
  {
    auto op = p.ops.at( i );
    if ( op.type == Operation::Type::LPB || op.type == Operation::Type::LPE )
    {
      continue;
    }
    p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 1 );
    bool can_remove = true;
    try
    {
      auto new_sequence = interpreter.eval( p );
      if ( new_sequence.size() != target_sequence.size() || new_sequence != target_sequence )
      {
        can_remove = false;
      }
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
  if ( removed_ops > 0 )
  {
    Log::get().debug( "Removed " + std::to_string( removed_ops ) + " operations during minimization" );
  }
  optimize( p, 1 );
}
