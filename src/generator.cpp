#include "generator.hpp"

#include "number.hpp"
#include "interpreter.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"

#include <algorithm>
#include <ctype.h>
#include <iostream>
#include <set>
#include <stdexcept>

#define POSITION_RANGE 100

State::State()
    :
    State( 0, 4, 4, 2, 3 )
{
}

void State::print()
{
  std::cout << "[";
  operation_dist.print();
  std::cout << ",";
  target_type_dist.print();
  std::cout << ",";
  target_value_dist.print();
  std::cout << ",";
  source_type_dist.print();
  std::cout << ",";
  source_value_dist.print();
  std::cout << ",";
  transition_dist.print();
  std::cout << ",";
  position_dist.print();
  std::cout << "]";
}

void Generator::print()
{
  std::cout << "[" << score << ",";
  for ( size_t i = 0; i < states.size(); i++ )
  {
    if ( i > 0 )
    {
      std::cout << ",";
    }
    states[i].print();
  }
  std::cout << "]";
}

State::State( size_t numStates, size_t maxConstant, size_t num_operation_types, size_t num_target_types,
    size_t num_source_types )
    :
    operation_dist( Distribution::uniform( num_operation_types ) ),
    target_type_dist( Distribution::exponential( num_target_types ) ),
    target_value_dist( Distribution::uniform( maxConstant + 1 ) ),
    source_type_dist( Distribution::exponential( num_source_types ) ),
    source_value_dist( Distribution::uniform( maxConstant + 1 ) ),
    transition_dist( Distribution::uniform( numStates ) ),
    position_dist( Distribution::uniform( POSITION_RANGE ) )
{
}

State State::operator+( const State &other )
{
  State r;
  r.operation_dist = Distribution::add( operation_dist, other.operation_dist );
  r.target_type_dist = Distribution::add( target_type_dist, other.target_type_dist );
  r.source_type_dist = Distribution::add( source_type_dist, other.source_type_dist );
  r.transition_dist = Distribution::add( transition_dist, other.transition_dist );
  r.position_dist = Distribution::add( position_dist, other.position_dist );
  return r;
}

Generator::Generator( const Settings &settings, size_t numStates, int64_t seed )
    :
    settings( settings )
{
  for ( char c : settings.operation_types )
  {
    c = tolower( c );
    switch ( c )
    {
    case 'a':
      operation_types.push_back( Operation::Type::ADD );
      break;
    case 's':
      operation_types.push_back( Operation::Type::SUB );
      break;
    case 'm':
      operation_types.push_back( Operation::Type::MOV );
      break;
    case 'u':
      operation_types.push_back( Operation::Type::MUL );
      break;
    case 'd':
      operation_types.push_back( Operation::Type::DIV );
      break;
    case 'l':
      operation_types.push_back( Operation::Type::LPB );
      break;
    default:
      Log::get().error( "Unknown operation type: " + std::to_string( c ), true );
    }
  }
  if ( operation_types.empty() )
  {
    Log::get().error( "No operation types", true );
  }

  // order of operand types is important as it defines its probability
  if ( settings.operand_types.find( 'd' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::MEM_ACCESS_DIRECT );
    target_operand_types.push_back( Operand::Type::MEM_ACCESS_DIRECT );
  }
  if ( settings.operand_types.find( 'c' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::CONSTANT );
  }
  if ( settings.operand_types.find( 'i' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::MEM_ACCESS_INDIRECT );
    target_operand_types.push_back( Operand::Type::MEM_ACCESS_INDIRECT );
  }
  if ( source_operand_types.empty() )
  {
    Log::get().error( "No source operation types", true );
  }
  if ( target_operand_types.empty() )
  {
    Log::get().error( "No source operation types", true );
  }

  if ( !settings.program_template.empty() )
  {
    Parser parser;
    program_template = parser.parse( settings.program_template );
  }

  states.resize( numStates );
  for ( number_t state = 0; state < numStates; state++ )
  {
    states[state] = State( numStates, settings.max_constant, operation_types.size(), target_operand_types.size(),
        source_operand_types.size() );
  }
  setSeed( seed );
}

Generator Generator::operator+( const Generator &other )
{
  Generator r( settings, states.size(), gen() );
  for ( number_t s = 0; s < states.size(); s++ )
  {
    r.states[s] = states[s] + other.states[s];
  }
  return r;
}

void Generator::mutate( double delta )
{
  for ( auto &s : states )
  {
    s.operation_dist = Distribution::mutate( s.operation_dist, gen );
    s.target_type_dist = Distribution::mutate( s.target_type_dist, gen );
    s.target_value_dist = Distribution::mutate( s.target_value_dist, gen );
    s.source_type_dist = Distribution::mutate( s.source_type_dist, gen );
    s.source_value_dist = Distribution::mutate( s.source_value_dist, gen );
    s.transition_dist = Distribution::mutate( s.transition_dist, gen );
    s.position_dist = Distribution::mutate( s.position_dist, gen );
  }
}

void Generator::generateOperations( Seed &seed )
{
  auto &s = states.at( seed.state );
  auto target_type = target_operand_types.at( s.target_type_dist( gen ) );
  auto source_type = source_operand_types.at( s.source_type_dist( gen ) );
  number_t target_value = s.target_value_dist( gen );
  number_t source_value = s.source_value_dist( gen );
  auto op_type = operation_types.at( s.operation_dist( gen ) );

  // bias for constant loop fragment length
  if ( op_type == Operation::Type::LPB && source_type != Operand::Type::CONSTANT && s.position_dist( gen ) % 10 > 0 )
  {
    source_type = Operand::Type::CONSTANT;
  }

  // avoid meaningless zeros or singularities
  if ( source_type == Operand::Type::CONSTANT && source_value == 0
      && (op_type == Operation::Type::ADD || op_type == Operation::Type::SUB || op_type == Operation::Type::DIV
          || op_type == Operation::Type::LPB) )
  {
    source_value = 1;
  }
  if ( source_type == Operand::Type::CONSTANT && op_type == Operation::Type::MUL )
  {
    source_value += 2;
  }

  seed.ops.clear();
  seed.ops.push_back(
      Operation( op_type, Operand( target_type, target_value ), Operand( source_type, source_value ) ) );
  if ( op_type == Operation::Type::LPB )
  {
    seed.ops.push_back( Operation( Operation::Type::LPE ) );
  }

  seed.position = static_cast<double>( s.position_dist( gen ) ) / POSITION_RANGE;
  seed.state = s.transition_dist( gen );
}

Program Generator::generateProgram( size_t initialState )
{
  // use template for base program
  Program p = program_template;

  // fill program with random operations
  Seed seed;
  seed.state = initialState;
  while ( p.ops.size() < settings.num_operations )
  {
    generateOperations( seed );
    size_t position = (seed.position * (p.ops.size() + 1));
    for ( size_t j = 0; j < seed.ops.size(); j++ )
    {
      p.ops.emplace( p.ops.begin() + position + j, std::move( seed.ops[j] ) );
    }
  }

  // fix causality of read operations
  std::set<number_t> written_cells;
  written_cells.insert( 0 );
  for ( size_t position = 0; position < p.ops.size(); position++ )
  {
    auto &op = p.ops[position];

    // fix source operands in new operation
    if ( op.source.type == Operand::Type::MEM_ACCESS_DIRECT || op.source.type == Operand::Type::MEM_ACCESS_INDIRECT )
    {
      int x = op.source.value % written_cells.size();
      for ( number_t cell : written_cells )
      {
        if ( x-- == 0 )
        {
          op.source.value = cell;
          break;
        }
      }
    }
    if ( op.type == Operation::Type::LPB )
    {
      int x = op.target.value % written_cells.size();
      for ( number_t cell : written_cells )
      {
        if ( x-- == 0 )
        {
          op.target.value = cell;
          break;
        }
      }
    }
    if ( op.type == Operation::Type::SUB && written_cells.find( op.target.value ) == written_cells.end() )
    {
      op.type = Operation::Type::ADD;
    }

    // update written cells
    switch ( op.type )
    {
    case Operation::Type::ADD:
    case Operation::Type::SUB:
    case Operation::Type::MOV:
    case Operation::Type::MUL:
    case Operation::Type::DIV:
    {
      if ( op.target.type == Operand::Type::MEM_ACCESS_DIRECT )
      {
        written_cells.insert( op.target.value );
      }
      break;
    }
    default:
      break;
    }

  }

  // make sure that the initial value does not get overridden immediately
  for ( auto it = p.ops.begin(); it < p.ops.end(); it++ )
  {
    if ( it->target.value == 0 )
    {
      if ( it->type == Operation::Type::MOV
          || (it->type == Operation::Type::SUB && (it->source.type != Operand::Type::CONSTANT && it->source.value == 0)) )
      {
        it = p.ops.erase( it );
      }
    }
    else if ( it->source.type != Operand::Type::CONSTANT && it->source.value == 0 )
    {
      break;
    }
  }

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
      if ( op.target.type == Operand::Type::MEM_ACCESS_DIRECT )
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
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::MEM_ACCESS_DIRECT, source ) ) );
  }

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
      num_ops++;
      break;
    case Operation::Type::SUB:
      can_descent = true;
      break;
    case Operation::Type::MOV:
    case Operation::Type::DIV:
      num_ops++;
      can_descent = true;
      break;
    case Operation::Type::LPE:
    {
      if ( !can_descent )
      {
        Operation sub( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, mem ),
            Operand( Operand::Type::CONSTANT, 1 ) );
        p.ops.insert( p.ops.begin() + i, sub );
        i++;
      }
      if ( num_ops == 0 )
      {
        size_t val = (seed.position * 5) + 1;
        Operation add( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, mem + 1 ),
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

  return p;
}

void Generator::setSeed( int64_t seed )
{
  gen.seed( seed );
}
