#include "generator.hpp"

#include "number.hpp"
#include "interpreter.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"

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

std::discrete_distribution<> operationDist( const ProgramUtil::Stats& stats,
    const std::vector<Operation::Type> &operation_types )
{
  std::vector<double> p( operation_types.size() );
  for ( size_t i = 0; i < operation_types.size(); i++ )
  {
    int64_t rate = stats.num_ops_per_type.at( static_cast<size_t>( operation_types[i] ) );
    if ( rate <= 0 )
    {
      Log::get().error( "Unexpected stats for operation type: " + Operation::Metadata::get( operation_types[i] ).name,
          true );
    }
    rate = std::max<int64_t>( rate / 1000, 1 );
    p[i] = rate;
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

Generator::Generator( const Settings &settings, int64_t seed )
    : settings( settings )
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

  std::vector<double> source_type_rates;
  std::vector<double> target_type_rates;
  if ( settings.operand_types.find( 'c' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::CONSTANT );
    source_type_rates.push_back( 4 );
  }
  if ( settings.operand_types.find( 'd' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::DIRECT );
    source_type_rates.push_back( 4 );
    target_operand_types.push_back( Operand::Type::DIRECT );
    target_type_rates.push_back( 4 );
  }
  if ( settings.operand_types.find( 'i' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::INDIRECT );
    source_type_rates.push_back( 1 );
    target_operand_types.push_back( Operand::Type::INDIRECT );
    target_type_rates.push_back( 1 );
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
  ProgramUtil::Stats stats;
  stats.load( "stats" );
  operation_dist = operationDist( stats, operation_types );
  target_type_dist = std::discrete_distribution<>( target_type_rates.begin(), target_type_rates.end() );
  target_value_dist = uniformDist( settings.max_constant + 1 );
  source_type_dist = std::discrete_distribution<>( source_type_rates.begin(), source_type_rates.end() );
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
          || op_type == Operation::Type::POW || op_type == Operation::Type::GCD || op_type == Operation::Type::BIN) )
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
      p.ops.emplace( p.ops.begin() + position, Operation( next_ops[j] ) );
      position = ((position + p.ops.size()) / 2) + 1;
    }
  }

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
        size_t val = (next_position * 5) + 1;
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

  return p;
}

void Generator::setSeed( int64_t seed )
{
  gen.seed( seed );
}
