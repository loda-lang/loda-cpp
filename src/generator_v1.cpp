#include "generator_v1.hpp"

#include "number.hpp"
#include "interpreter.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "stats.hpp"

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

std::discrete_distribution<> operationDist( const Stats &stats, const std::vector<Operation::Type> &operation_types )
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

std::discrete_distribution<> constantsDist( const std::vector<number_t> &constants, const Stats &stats )
{
  std::vector<double> p( constants.size() );
  for ( size_t i = 0; i < constants.size(); i++ )
  {
    int64_t rate = stats.num_constants.at( constants[i] );
    if ( rate <= 0 )
    {
      Log::get().error( "Unexpected stats for constant: " + std::to_string( constants[i] ), true );
    }
    p[i] = rate;
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

GeneratorV1::GeneratorV1( const Settings &settings, int64_t seed )
    :
    Generator( settings, seed )
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
  Stats stats;
  stats.load( "stats" );
  constants.resize( stats.num_constants.size() );
  size_t i = 0;
  for ( auto &c : stats.num_constants )
  {
    constants[i++] = c.first;
  }

  constants_dist = constantsDist( constants, stats );
  operation_dist = operationDist( stats, operation_types );
  target_type_dist = std::discrete_distribution<>( target_type_rates.begin(), target_type_rates.end() );
  target_value_dist = uniformDist( settings.max_constant + 1 );
  source_type_dist = std::discrete_distribution<>( source_type_rates.begin(), source_type_rates.end() );
  source_value_dist = uniformDist( settings.max_constant + 1 );
  position_dist = uniformDist( POSITION_RANGE );
}

std::pair<Operation, double> GeneratorV1::generateOperation()
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

  // use constants distribution from stats
  if ( source_type == Operand::Type::CONSTANT )
  {
    source_value = constants.at( constants_dist( gen ) );
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

  std::pair<Operation, double> next_op;
  next_op.first = Operation( op_type, Operand( target_type, target_value ), Operand( source_type, source_value ) );
  next_op.second = static_cast<double>( position_dist( gen ) ) / POSITION_RANGE;
  return next_op;
}

Program GeneratorV1::generateProgram()
{
  // use template for base program
  Program p = program_template;
  generateStateless( p, settings.num_operations );
  applyPostprocessing( p );
  return p;
}
