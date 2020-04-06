#include "optimizer.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "semantics.hpp"
#include "util.hpp"

void Optimizer::optimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells ) const
{
  Log::get().debug( "Starting optimization of program with " + std::to_string( p.ops.size() ) + " operations" );
  bool changed = true;
  while ( changed )
  {
    changed = false;
    if ( simplifyOperations( p, num_initialized_cells ) )
    {
      changed = true;
    }
    if ( mergeOps( p ) )
    {
      changed = true;
    }
    if ( removeNops( p ) )
    {
      changed = true;
    }
    if ( removeEmptyLoops( p ) )
    {
      changed = true;
    }
    if ( reduceMemoryCells( p, num_reserved_cells ) )
    {
      changed = true;
    }
    if ( evalConstants( p, num_initialized_cells ) )
    {
      changed = true;
    }
  }
  Log::get().debug( "Finished optimization; program now has " + std::to_string( p.ops.size() ) + " operations" );
}

bool Optimizer::removeNops( Program &p ) const
{
  Log::get().debug( "Removing NOP operations" );
  bool removed = false;
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    auto t = it->type;
    if ( t == Operation::Type::NOP || t == Operation::Type::DBG
        || ((t == Operation::Type::ADD || t == Operation::Type::SUB) && it->source.type == Operand::Type::CONSTANT
            && it->source.value == 0)
        || ((t == Operation::Type::MOV || t == Operation::Type::GCD) && it->source == it->target)
        || ((t == Operation::Type::MUL || t == Operation::Type::DIV || t == Operation::Type::POW)
            && it->source.type == Operand::Type::CONSTANT && it->source.value == 1) )
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

bool Optimizer::removeEmptyLoops( Program &p ) const
{
  Log::get().debug( "Removing empty loops" );
  bool removed = false;
  for ( int i = 0; i < static_cast<int>( p.ops.size() ); i++ ) // need to use signed integers here
  {
    if ( i + 1 < static_cast<int>( p.ops.size() ) && p.ops[i].type == Operation::Type::LPB
        && p.ops[i + 1].type == Operation::Type::LPE )
    {
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 2 );
      i = i - 2;
      removed = true;
    }
  }
  return removed;
}

bool Optimizer::mergeOps( Program &p ) const
{
  Log::get().debug( "Merging operations" );
  bool merged = false;
  for ( size_t i = 0; i + 1 < p.ops.size(); i++ )
  {
    bool do_merge = false;

    auto &o1 = p.ops[i];
    auto &o2 = p.ops[i + 1];

    // operation targets the direct same?
    if ( o1.target == o2.target && o1.target.type == Operand::Type::DIRECT )
    {
      // both sources constants?
      if ( o1.source.type == Operand::Type::CONSTANT && o2.source.type == Operand::Type::CONSTANT )
      {
        // both add or sub operation?
        if ( o1.type == o2.type && (o1.type == Operation::Type::ADD || o1.type == Operation::Type::SUB) )
        {
          o1.source.value += o2.source.value;
          do_merge = true;
        }

        // both mul, div or pow operations?
        if ( o1.type == o2.type
            && (o1.type == Operation::Type::MUL || o1.type == Operation::Type::DIV || o1.type == Operation::Type::POW) )
        {
          o1.source.value *= o2.source.value;
          do_merge = true;
        }

        // first add, second sub?
        else if ( o1.type == Operation::Type::ADD && o2.type == Operation::Type::SUB )
        {
          if ( o1.source.value > o2.source.value )
          {
            o1.source.value = o1.source.value - o2.source.value;
            o1.type = Operation::Type::ADD;
          }
          else
          {
            o1.source.value = o2.source.value - o1.source.value;
            o1.type = Operation::Type::SUB;
          }
          do_merge = true;
        }

        // both sources same memory cell?
        else if ( o1.source.type == Operand::Type::DIRECT && o1.source == o2.source )
        {
          // add with inverse sub?
          if ( o1.type == Operation::Type::ADD && o2.type == Operation::Type::SUB )
          {
            o1.source = Operand( Operand::Type::CONSTANT, 0 );
            do_merge = true;
          }
        }

      }

      // second operation mov with constant?
      if ( o2.type == Operation::Type::MOV && o2.source.type == Operand::Type::CONSTANT )
      {
        // first operation writing target?
        if ( Operation::Metadata::get( o1.type ).is_writing_target )
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

inline bool simplifyOperand( Operand &op, std::unordered_set<number_t> &initialized_cells, bool is_source )
{
  Log::get().debug( "Simplifying operand" );
  switch ( op.type )
  {
  case Operand::Type::CONSTANT:
    break;
  case Operand::Type::DIRECT:
    if ( initialized_cells.find( op.value ) == initialized_cells.end() && is_source )
    {
      op.type = Operand::Type::CONSTANT;
      op.value = 0;
      return true;
    }
    break;
  case Operand::Type::INDIRECT:
    if ( initialized_cells.find( op.value ) == initialized_cells.end() )
    {
      op.type = Operand::Type::DIRECT;
      op.value = 0;
      return true;
    }
    break;
  }
  return false;
}

bool Optimizer::simplifyOperations( Program &p, size_t num_initialized_cells ) const
{
  Log::get().debug( "Simplifying operations" );
  std::unordered_set<number_t> initialized_cells;
  for ( number_t i = 0; i < num_initialized_cells; ++i )
  {
    initialized_cells.insert( i );
  }
  bool simplified = false;
  bool can_simplify = true;
  for ( auto &op : p.ops )
  {
    switch ( op.type )
    {
    case Operation::Type::MOV:
    case Operation::Type::ADD:
    case Operation::Type::SUB:
    case Operation::Type::MUL:
    case Operation::Type::DIV:
    case Operation::Type::MOD:
    case Operation::Type::POW:
    case Operation::Type::FAC:
    case Operation::Type::GCD:
    {
      if ( can_simplify )
      {
        // simplify operands
        bool has_source = Operation::Metadata::get( op.type ).num_operands == 2;
        if ( has_source && simplifyOperand( op.source, initialized_cells, true ) )
        {
          simplified = true;
        }
        if ( simplifyOperand( op.target, initialized_cells, false ) )
        {
          simplified = true;
        }

        // simplify operation (cell content matters!)
        if ( op.target.type == Operand::Type::DIRECT
            && initialized_cells.find( op.target.value ) == initialized_cells.end() )
        {
          // add $n,X => mov $n,X (where $n is uninitialized)
          if ( op.type == Operation::Type::ADD )
          {
            op.type = Operation::Type::MOV;
            simplified = true;
          }
          // fac $n => mov $n,1 (where $n is uninitialized)
          else if ( op.type == Operation::Type::FAC )
          {
            op.type = Operation::Type::MOV;
            op.source = Operand( Operand::Type::CONSTANT, 1 );
            simplified = true;
          }
        }
      }

      // simplify operation (cell content doesn't matter)
      if ( op.target.type == Operand::Type::DIRECT && op.target == op.source )
      {
        // add $n,$n => mul $n,2
        if ( op.type == Operation::Type::ADD )
        {
          op.type = Operation::Type::MUL;
          op.source = Operand( Operand::Type::CONSTANT, 2 );
          simplified = true;
        }
        // mul $n,$n => pow $n,2
        else if ( op.type == Operation::Type::MUL )
        {
          op.type = Operation::Type::POW;
          op.source = Operand( Operand::Type::CONSTANT, 2 );
          simplified = true;
        }
      }

      // update initialized cells
      switch ( op.target.type )
      {
      case Operand::Type::DIRECT:
        initialized_cells.insert( op.target.value );
        break;
      case Operand::Type::INDIRECT:
        can_simplify = false; // don't know at this point which cell is written to
        break;
      case Operand::Type::CONSTANT:
        Log::get().error( "invalid program" );
        break;
      }

      break;
    }

    case Operation::Type::NOP:
    case Operation::Type::DBG:
      break; // can be safely ignored

    case Operation::Type::LPB:
    case Operation::Type::LPE:
    case Operation::Type::CLR:
      can_simplify = false;
      break;

    }
  }
  return simplified;
}

bool Optimizer::getUsedMemoryCells( const Program &p, std::unordered_set<number_t> &used_cells,
    number_t &largest_used ) const
{
  for ( auto &op : p.ops )
  {
    size_t region_length = 1;
    if ( op.source.type == Operand::Type::INDIRECT || op.target.type == Operand::Type::INDIRECT )
    {
      return false;
    }
    if ( op.type == Operation::Type::LPB )
    {
      if ( op.source.type == Operand::Type::CONSTANT )
      {
        region_length = op.source.value;
      }
      else
      {
        return false;
      }
    }
    if ( op.source.type == Operand::Type::DIRECT )
    {
      for ( size_t i = 0; i < region_length; i++ )
      {
        used_cells.insert( op.source.value + i );
      }
    }
    if ( op.target.type == Operand::Type::DIRECT )
    {
      for ( size_t i = 0; i < region_length; i++ )
      {
        used_cells.insert( op.target.value + i );
      }
    }
  }
  largest_used = used_cells.empty() ? 0 : *used_cells.begin();
  for ( number_t used : used_cells )
  {
    largest_used = std::max( largest_used, used );
  }
  return true;
}

bool Optimizer::reduceMemoryCells( Program &p, size_t num_reserved_cells ) const
{
  Log::get().debug( "Reducing memory cells" );
  std::unordered_set<number_t> used_cells;
  number_t largest_used = 0;
  if ( !getUsedMemoryCells( p, used_cells, largest_used ) )
  {
    return false;
  }
  for ( number_t candidate = 0; candidate < largest_used; ++candidate )
  {
    bool free = true;
    if ( candidate < num_reserved_cells )
    {
      free = false;
    }
    for ( number_t used : used_cells )
    {
      if ( used == candidate )
      {
        free = false;
        break;
      }
    }
    if ( free )
    {
      bool replaced = false;
      for ( auto &op : p.ops )
      {
        if ( op.source.type == Operand::Type::DIRECT && op.source.value == largest_used )
        {
          op.source.value = candidate;
          replaced = true;
        }
        if ( op.target.type == Operand::Type::DIRECT && op.target.value == largest_used )
        {
          op.target.value = candidate;
          replaced = true;
        }
      }
      return replaced;
    }
  }
  return false;
}

struct update_state
{
  bool changed;
  bool stop;
};

update_state updateConstantsArithmetic( Operation &op, std::unordered_map<number_t, number_t> &values,
    std::unordered_set<number_t> &unknown_cells )
{
  // make sure there is not indirect memory access
  auto num_ops = Operation::Metadata::get( op.type ).num_operands;
  if ( (num_ops > 0 && op.target.type == Operand::Type::INDIRECT)
      || (num_ops > 1 && op.source.type == Operand::Type::INDIRECT) )
  {
    update_state result;
    result.changed = false;
    result.stop = true;
    return result;
  }

  bool unknown = false;

  // deduce source value
  number_t source_value = 0;
  if ( num_ops > 1 )
  {
    if ( op.source.type == Operand::Type::CONSTANT )
    {
      source_value = op.source.value;
    }
    else // direct
    {
      if ( unknown_cells.find( op.source.value ) != unknown_cells.end() )
      {
        unknown = true;
      }
      else
      {
        auto found = values.find( op.source.value );
        if ( found != values.end() )
        {
          source_value = found->second;
        }
        op.source = Operand( Operand::Type::CONSTANT, source_value );
      }
    }
  }

  // deduce target value
  number_t target_value = 0;
  if ( unknown_cells.find( op.target.value ) != unknown_cells.end() && op.type != Operation::Type::MOV )
  {
    unknown = true;
  }
  else
  {
    auto found = values.find( op.target.value );
    if ( found != values.end() )
    {
      target_value = found->second;
    }
  }

  // calculate new value
  bool update = true;
  switch ( op.type )
  {
  case Operation::Type::MOV:
    target_value = source_value;
    break;
  case Operation::Type::ADD:
    target_value = Semantics::add( target_value, source_value );
    break;
  case Operation::Type::SUB:
    target_value = Semantics::sub( target_value, source_value );
    break;
  case Operation::Type::MUL:
    target_value = Semantics::mul( target_value, source_value );
    break;
  case Operation::Type::DIV:
    target_value = Semantics::div( target_value, source_value );
    break;
  case Operation::Type::MOD:
    target_value = Semantics::mod( target_value, source_value );
    break;
  case Operation::Type::POW:
    target_value = Semantics::pow( target_value, source_value );
    break;
  case Operation::Type::FAC:
    target_value = Semantics::fac( target_value );
    break;
  case Operation::Type::GCD:
    target_value = Semantics::gcd( target_value, source_value );
    break;

  case Operation::Type::NOP:
  case Operation::Type::DBG:
    update = false;
    break;

  case Operation::Type::LPB:
  case Operation::Type::LPE:
  case Operation::Type::CLR:
  {
    update_state result;
    result.changed = false;
    result.stop = true;
    return result;
  }
  }

  update_state result;
  result.changed = false;
  result.stop = false;

  if ( unknown )
  {
    unknown_cells.insert( op.target.value );
  }
  else if ( update )
  {
    values[op.target.value] = target_value;
    if ( op.source.type == Operand::Type::DIRECT )
    {
      op.type = Operation::Type::MOV;
      op.source = Operand( Operand::Type::CONSTANT, target_value );
      result.changed = true;
    }
  }
  return result;
}

bool Optimizer::evalConstants( Program &p, size_t num_initialized_cells ) const
{
  std::unordered_map<number_t, number_t> values;
  std::unordered_set<number_t> unknown_cells;
  for ( size_t i = 0; i < num_initialized_cells; i++ )
  {
    unknown_cells.insert( i );
  }
  bool changed = false;
  for ( auto &op : p.ops )
  {
    auto state = updateConstantsArithmetic( op, values, unknown_cells );
    if ( state.stop )
    {
      return changed;
    }
    changed = changed || state.changed;
  }
  return changed;
}

void Optimizer::minimize( Program &p, size_t num_terms ) const
{
  Log::get().debug( "Minimizing program" );
  Interpreter interpreter( settings );
  Sequence target_sequence;
  try
  {
    target_sequence = interpreter.eval( p, num_terms );
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
    else if ( op.type == Operation::Type::LPB )
    {
      if ( op.source.type != Operand::Type::CONSTANT || op.source.value != 1 )
      {
        p.ops.at( i ).source = Operand( Operand::Type::CONSTANT, 1 );
        bool can_reset = true;
        try
        {
          auto new_sequence = interpreter.eval( p, num_terms );
          if ( new_sequence.size() != target_sequence.size() || new_sequence != target_sequence )
          {
            can_reset = false;
          }
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
      bool can_remove = true;
      try
      {
        auto new_sequence = interpreter.eval( p, num_terms );
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
  }
  if ( removed_ops > 0 )
  {
    Log::get().debug( "Removed " + std::to_string( removed_ops ) + " operations during minimization" );
  }
}
