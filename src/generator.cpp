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

std::discrete_distribution<> uniformDist( size_t size )
{
  std::vector<double> p( size, 100.0 );
  return std::discrete_distribution<>( p.begin(), p.end() );
}

std::discrete_distribution<> exponentialDist( size_t size )
{
  std::vector<double> p( size );
  double v = 1.0;
  for ( int i = size - 1; i >= 0; --i )
  {
    p[i] = v;
    v *= 2.0;
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

Generator::Generator( const Settings &settings, int64_t seed )
    :
    settings( settings )
{
  // parse operation types
  bool negate = false;
  for ( char c : settings.operation_types )
  {
    c = tolower( c );
    if ( c == '^' )
    {
      negate = true;
    }
    else
    {
      auto type = Operation::Type::NOP;
      for ( auto &t : Operation::Types )
      {
        auto &m = Operation::Metadata::get( t );
        if ( m.is_public && m.short_name == c )
        {
          type = t;
          break;
        }
      }
      if ( type == Operation::Type::NOP )
      {
        Log::get().error( "Unknown operation type: " + std::string( 1, c ), true );
      }
      if ( type != Operation::Type::LPE )
      {
        operation_types.push_back( type );
      }
    }
  }
  if ( negate )
  {
    std::vector<Operation::Type> tmp_types;
    for ( auto t : Operation::Types )
    {
      bool found = false;
      for ( auto o : operation_types )
      {
        if ( o == t )
        {
          found = true;
          break;
        }
      }
      if ( !found && Operation::Metadata::get( t ).is_public && t != Operation::Type::LPE )
      {
        tmp_types.push_back( t );
      }
    }
    operation_types = tmp_types;
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
    Log::get().error( "No target operation types", true );
  }

  // program template
  if ( !settings.program_template.empty() )
  {
    Parser parser;
    program_template = parser.parse( settings.program_template );
  }

  // initialize distributions
  operation_dist = uniformDist( operation_types.size() );
  target_type_dist = exponentialDist( target_operand_types.size() );
  target_value_dist = uniformDist( settings.max_constant + 1 );
  source_type_dist = exponentialDist( source_operand_types.size() );
  source_value_dist = uniformDist( settings.max_constant + 1 );
  position_dist = uniformDist( POSITION_RANGE );

  setSeed( seed );
}

void Generator::generateOperations()
{
  auto target_type = target_operand_types.at( target_type_dist( gen ) );
  auto source_type = source_operand_types.at( source_type_dist( gen ) );
  number_t target_value = target_value_dist( gen );
  number_t source_value = source_value_dist( gen );
  auto op_type = operation_types.at( operation_dist( gen ) );

  // check number of operands
  if ( Operation::Metadata::get( op_type ).num_operands < 2 )
  {
    source_type = Operand::Type::CONSTANT;
    source_value = 0;
  }
  if ( Operation::Metadata::get( op_type ).num_operands < 1 )
  {
    target_type = Operand::Type::CONSTANT;
    target_value = 0;
  }

  // bias for constant loop fragment length
  if ( op_type == Operation::Type::LPB && source_type != Operand::Type::CONSTANT && position_dist( gen ) % 10 > 0 )
  {
    source_type = Operand::Type::CONSTANT;
  }

  // avoid meaningless zeros or singularities
  if ( source_type == Operand::Type::CONSTANT && source_value < 1
      && (op_type == Operation::Type::ADD || op_type == Operation::Type::SUB || op_type == Operation::Type::LPB) )
  {
    source_value = 1;
  }
  if ( source_type == Operand::Type::CONSTANT && source_value < 2
      && (op_type == Operation::Type::MUL || op_type == Operation::Type::DIV || op_type == Operation::Type::MOD
          || op_type == Operation::Type::POW || op_type == Operation::Type::GCD) )
  {
    source_value = 2;
  }

  next_ops.clear();
  next_ops.push_back(
      Operation( op_type, Operand( target_type, target_value ), Operand( source_type, source_value ) ) );
  if ( op_type == Operation::Type::LPB )
  {
    next_ops.push_back( Operation( Operation::Type::LPE ) );
  }

  next_position = static_cast<double>( position_dist( gen ) ) / POSITION_RANGE;
}

Program Generator::generateProgram()
{
  // use template for base program
  Program p = program_template;

  // fill program with random operations
  while ( p.ops.size() < settings.num_operations )
  {
    generateOperations();
    size_t position = (next_position * (p.ops.size() + 1));
    for ( size_t j = 0; j < next_ops.size(); j++ )
    {
      p.ops.emplace( p.ops.begin() + position + j, Operation( next_ops[j] ) );
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
    if ( Operation::Metadata::get( op.type ).is_writing_target && op.target.type == Operand::Type::MEM_ACCESS_DIRECT )
    {
      written_cells.insert( op.target.value );
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
    case Operation::Type::POW:
    case Operation::Type::FAC:
      num_ops++;
      break;
    case Operation::Type::SUB:
      can_descent = true;
      break;
    case Operation::Type::MOV:
    case Operation::Type::DIV:
    case Operation::Type::MOD:
    case Operation::Type::GCD:
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
        size_t val = (next_position * 5) + 1;
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
