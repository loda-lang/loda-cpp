#include "generator.hpp"

#include "number.hpp"
#include "interpreter.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"

#include <algorithm>
#include <set>

#define POSITION_RANGE 100

State::State()
    : State( 0, 4, 4, 2, 3 )
{
}

void State::print()
{
  std::cout << "[";
  printDist( operation_dist );
  std::cout << ",";
  printDist( target_type_dist );
  std::cout << ",";
  printDist( target_value_dist );
  std::cout << ",";
  printDist( source_type_dist );
  std::cout << ",";
  printDist( source_value_dist );
  std::cout << ",";
  printDist( transition_dist );
  std::cout << ",";
  printDist( position_dist );
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
    : operation_dist( EqualDist( num_operation_types ) ),
      target_type_dist( ExpDist( num_target_types ) ),
      target_value_dist( EqualDist( maxConstant + 1 ) ),
      source_type_dist( ExpDist( num_source_types ) ),
      source_value_dist( EqualDist( maxConstant + 1 ) ),
      transition_dist( EqualDist( numStates ) ),
      position_dist( EqualDist( POSITION_RANGE ) )
{
}

State State::operator+( const State& other )
{
  State r;
  r.operation_dist = AddDist( operation_dist, other.operation_dist );
  r.target_type_dist = AddDist( target_type_dist, other.target_type_dist );
  r.source_type_dist = AddDist( source_type_dist, other.source_type_dist );
  r.transition_dist = AddDist( transition_dist, other.transition_dist );
  r.position_dist = AddDist( position_dist, other.position_dist );
  return r;
}

Generator::Generator( const Settings& settings, size_t numStates, int64_t seed )
    : settings( settings )
{
  for ( char c : settings.operation_types )
  {
    switch ( c )
    {
    case 'a':
    case 'A':
      operation_types.push_back( Operation::Type::ADD );
      break;
    case 's':
    case 'S':
      operation_types.push_back( Operation::Type::SUB );
      break;
    case 'm':
    case 'M':
      operation_types.push_back( Operation::Type::MOV );
      break;
    case 'l':
    case 'L':
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

  for ( char c : settings.operand_types )
  {
    switch ( c )
    {
    case 'c':
    case 'C':
      source_operand_types.push_back( Operand::Type::CONSTANT );
      break;
    case 'd':
    case 'D':
      source_operand_types.push_back( Operand::Type::MEM_ACCESS_DIRECT );
      target_operand_types.push_back( Operand::Type::MEM_ACCESS_DIRECT );
      break;
    case 'i':
    case 'I':
      source_operand_types.push_back( Operand::Type::MEM_ACCESS_INDIRECT );
      target_operand_types.push_back( Operand::Type::MEM_ACCESS_INDIRECT );
      break;
    default:
      Log::get().error( "Unknown operand type: " + std::to_string( c ), true );
    }
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

Generator Generator::operator+( const Generator& other )
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
  for ( auto& s : states )
  {
    MutateDist( s.operation_dist, gen );
    MutateDist( s.target_type_dist, gen );
    MutateDist( s.target_value_dist, gen );
    MutateDist( s.source_type_dist, gen );
    MutateDist( s.source_value_dist, gen );
    MutateDist( s.transition_dist, gen );
    MutateDist( s.position_dist, gen );
  }
}

void Generator::generateOperations( Seed& seed )
{
  auto& s = states.at( seed.state );
  auto target_type = target_operand_types.at( s.target_type_dist( gen ) );
  auto source_type = source_operand_types.at( s.source_type_dist( gen ) );
  number_t target_value = s.target_value_dist( gen );
  number_t source_value = s.source_value_dist( gen );
  auto op_type = operation_types.at( s.operation_dist( gen ) );
  if ( source_type == Operand::Type::CONSTANT && source_value == 0
      && (op_type == Operation::Type::ADD || op_type == Operation::Type::SUB || op_type == Operation::Type::LPB) )
  {
    source_value = 1;
  }

  seed.ops.clear();
  seed.ops.push_back( Operation( op_type, Operand( target_type, target_value ), Operand( source_type, source_value ) ) );
  if ( op_type == Operation::Type::LPB )
  {
    seed.ops.push_back( Operation( Operation::Type::LPE ) );
  }

  seed.position = static_cast<double>( s.position_dist( gen ) ) / POSITION_RANGE;
  seed.state = s.transition_dist( gen );
}

Program Generator::generateProgram( size_t initialState )
{
  Program p = program_template;
  Seed seed;
  seed.state = initialState;
  while ( p.ops.size() < settings.num_operations )
  {
    generateOperations( seed );
    size_t position = (seed.position * (p.ops.size() + 1));

    // determine written cells so far
    std::set<number_t> written_cells;
    written_cells.insert( 0 );
    size_t open_loops = 0;
    size_t i = 0;
    while ( i < position || open_loops > 0 )
    {
      switch ( p.ops[i].type )
      {
      case Operation::Type::ADD:
      case Operation::Type::SUB:
      case Operation::Type::MOV:
      {
        if ( p.ops[i].target.type == Operand::Type::MEM_ACCESS_DIRECT )
        {
          written_cells.insert( p.ops[i].target.value );
        }
        break;
      }
      case Operation::Type::LPB:
        ++open_loops;
        break;
      case Operation::Type::LPE:
        --open_loops;
        break;
      default:
        break;
      }
      ++i;
    }

    for ( size_t j = 0; j < seed.ops.size(); j++ )
    {
      // fix source operands in new operation
      auto new_op = seed.ops[j];
      if ( new_op.source.type == Operand::Type::MEM_ACCESS_DIRECT
          || new_op.source.type == Operand::Type::MEM_ACCESS_INDIRECT )
      {
        int x = new_op.source.value % written_cells.size();
        for ( number_t cell : written_cells )
        {
          if ( x-- == 0 )
          {
            new_op.source.value = cell;
            break;
          }
        }
      }

      if ( new_op.type == Operation::Type::LPB )
      {
        int x = new_op.target.value % written_cells.size();
        for ( number_t cell : written_cells )
        {
          if ( x-- == 0 )
          {
            new_op.target.value = cell;
            break;
          }
        }
      }

      // add operation
      p.ops.emplace( p.ops.begin() + position + j, std::move( new_op ) );
    }
  }
  return p;
}

void Generator::setSeed( int64_t seed )
{
  gen.seed( seed );
}
