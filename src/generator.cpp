#include "generator.hpp"

Generator::Generator( const Settings &settings, int64_t seed )
    :
    settings( settings )
{
  gen.seed( seed );
}

std::vector<number_t> Generator::fixCausality( Program &p )
{
  // fix causality of read operations
  std::vector<number_t> written_cells;
  written_cells.push_back( 0 );
  for ( size_t position = 0; position < p.ops.size(); position++ )
  {
    auto &op = p.ops[position];
    auto &meta = Operation::Metadata::get( op.type );

    // fix source operand in new operation
    if ( meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT )
    {
      op.source.value = written_cells[op.source.value % written_cells.size()];
    }

    // check if target cell not written yet
    if ( meta.is_writing_target && op.target.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.target.value ) == written_cells.end() )
    {
      if ( meta.is_reading_target )
      {
        op.type = Operation::Type::MOV;
        if ( op.target == op.source )
        {
          op.target.value++;
        }
      }

      // update written cells
      written_cells.push_back( op.target.value );
    }
  }
  return written_cells;
}

void Generator::ensureSourceNotOverwritten( Program &p )
{
  // make sure that the initial value does not get overridden immediately
  for ( auto it = p.ops.begin(); it < p.ops.end(); it++ )
  {
    if ( it->target.value == 0 )
    {
      if ( it->type == Operation::Type::MOV
          || ((it->type == Operation::Type::SUB || it->type == Operation::Type::TRN)
              && (it->source.type != Operand::Type::CONSTANT && it->source.value == 0)) )
      {
        it = p.ops.erase( it );
      }
    }
    else if ( it->source.type != Operand::Type::CONSTANT && it->source.value == 0 )
    {
      break;
    }
  }
}

void Generator::ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells )
{
  // make sure that the target value gets written
  bool written = false;
  for ( auto &op : p.ops )
  {
    switch ( op.type )
    {
    case Operation::Type::ADD:
    case Operation::Type::SUB:
    case Operation::Type::MOV:
    {
      if ( op.target.type == Operand::Type::DIRECT )
      {
        if ( op.target.value == 1 )
        {
          if ( !written && op.type == Operation::Type::SUB )
          {
            op.type = Operation::Type::ADD;
          }
          written = true;
        }
      }
      break;
    }
    default:
      break;
    }
    if ( written ) break;
  }
  if ( !written )
  {
    number_t source = 0;
    for ( number_t cell : written_cells )
    {
      source = cell;
    }
    p.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::DIRECT, 1 ),
            Operand( Operand::Type::DIRECT, source ) ) );
  }
}

void Generator::ensureMeaningfulLoops( Program &p )
{
  // make sure loops do something
  number_t mem = 0;
  number_t num_ops = 0;
  bool can_descent = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    switch ( p.ops[i].type )
    {
    case Operation::Type::LPB:
    {
      mem = p.ops[i].target.value;
      can_descent = false;
      num_ops = 0;
      break;
    }
    case Operation::Type::ADD:
    case Operation::Type::MUL:
    case Operation::Type::POW:
    case Operation::Type::FAC:
      num_ops++;
      break;
    case Operation::Type::SUB:
    case Operation::Type::LOG:
      can_descent = true;
      break;
    case Operation::Type::MOV:
    case Operation::Type::DIV:
    case Operation::Type::MOD:
    case Operation::Type::GCD:
    case Operation::Type::BIN:
    case Operation::Type::CMP:
      num_ops++;
      can_descent = true;
      break;
    case Operation::Type::LPE:
    {
      if ( !can_descent )
      {
        Operation sub( Operation::Type::SUB, Operand( Operand::Type::DIRECT, mem ),
            Operand( Operand::Type::CONSTANT, 1 ) );
        p.ops.insert( p.ops.begin() + i, sub );
        i++;
      }
      if ( num_ops == 0 )
      {
        number_t val = (gen() * 5) + 1;
        Operation add( Operation::Type::ADD, Operand( Operand::Type::DIRECT, mem + 1 ),
            Operand( Operand::Type::CONSTANT, val ) );
        p.ops.insert( p.ops.begin() + i, add );
        i++;
      }
      break;
    }
    default:
      break;
    }
  }
}

void Generator::generateStateless( Program &p, size_t num_operations )
{
  // fill program with random operations
  while ( p.ops.size() < num_operations )
  {
    auto next_op = generateOperation();
    size_t position = (next_op.second * (p.ops.size() + 1));
    p.ops.emplace( p.ops.begin() + position, Operation( next_op.first ) );
    if ( next_op.first.type == Operation::Type::LPB )
    {
      position = ((position + p.ops.size()) / 2) + 1;
      p.ops.emplace( p.ops.begin() + position, Operation( Operation::Type::LPE ) );
    }
  }
}
