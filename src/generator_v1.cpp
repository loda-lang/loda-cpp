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

GeneratorV1::GeneratorV1( const Config &config, const Stats &stats, int64_t seed )
    : Generator( config, stats, seed ),
      num_operations( config.length )
{
  std::string operation_types = "^"; // negate operation types (exclusion pattern)
  if ( !config.loops )
  {
    operation_types += "l";
  }
  if ( !config.calls )
  {
    operation_types += "c";
  }
  std::string operand_types = config.indirect_access ? "cdi" : "cd";

  // parse operation types
  bool negate = false;
  for ( char c : operation_types )
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
        this->operation_types.push_back( type );
      }
    }
  }
  if ( negate )
  {
    std::vector<Operation::Type> tmp_types;
    for ( auto t : Operation::Types )
    {
      bool found = false;
      for ( auto o : this->operation_types )
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
    this->operation_types = tmp_types;
  }
  if ( operation_types.empty() )
  {
    Log::get().error( "No operation types", true );
  }

  std::vector<double> source_type_rates;
  std::vector<double> target_type_rates;
  if ( operand_types.find( 'c' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::CONSTANT );
    source_type_rates.push_back( 4 );
  }
  if ( operand_types.find( 'd' ) != std::string::npos )
  {
    source_operand_types.push_back( Operand::Type::DIRECT );
    source_type_rates.push_back( 4 );
    target_operand_types.push_back( Operand::Type::DIRECT );
    target_type_rates.push_back( 4 );
  }
  if ( operand_types.find( 'i' ) != std::string::npos )
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
  if ( !config.program_template.empty() )
  {
    Parser parser;
    this->program_template = parser.parse( config.program_template );
  }

  // initialize distributions
  constants.resize( stats.num_constants.size() );
  size_t i = 0;
  for ( auto &c : stats.num_constants )
  {
    constants[i++] = c.first;
  }

  constants_dist = constantsDist( constants, stats );
  operation_dist = operationDist( stats, this->operation_types );
  target_type_dist = std::discrete_distribution<>( target_type_rates.begin(), target_type_rates.end() );
  target_value_dist = uniformDist( config.max_constant + 1 );
  source_type_dist = std::discrete_distribution<>( source_type_rates.begin(), source_type_rates.end() );
  source_value_dist = uniformDist( config.max_constant + 1 );
  position_dist = uniformDist( POSITION_RANGE );
}

std::pair<Operation, double> GeneratorV1::generateOperation()
{
  Operation op;
  op.type = operation_types.at( operation_dist( gen ) );
  op.target.type = target_operand_types.at( target_type_dist( gen ) );
  op.target.value = target_value_dist( gen );
  op.source.type = source_operand_types.at( source_type_dist( gen ) );
  op.source.value = source_value_dist( gen );

  // check number of operands
  if ( Operation::Metadata::get( op.type ).num_operands < 2 )
  {
    op.source.type = Operand::Type::CONSTANT;
    op.source.value = 0;
  }
  if ( Operation::Metadata::get( op.type ).num_operands < 1 )
  {
    op.target.type = Operand::Type::CONSTANT;
    op.target.value = 0;
  }

  // bias for constant loop fragment length
  if ( op.type == Operation::Type::LPB && op.source.type != Operand::Type::CONSTANT && position_dist( gen ) % 10 > 0 )
  {
    op.source.type = Operand::Type::CONSTANT;
  }

  // use constants distribution from stats
  if ( op.source.type == Operand::Type::CONSTANT )
  {
    op.source.value = constants.at( constants_dist( gen ) );
  }

  // avoid meaningless zeros or singularities
  if ( op.source.type == Operand::Type::CONSTANT )
  {
    if ( op.source.value == 0
        && (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB || op.type == Operation::Type::LPB) )
    {
      op.source.value = 1;
    }
    if ( (op.source.value == 0 || op.source.value == 1)
        && (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV || op.type == Operation::Type::DIF
            || op.type == Operation::Type::MOD || op.type == Operation::Type::POW || op.type == Operation::Type::GCD
            || op.type == Operation::Type::BIN || op.type == Operation::Type::LOG) )
    {
      op.source.value = 2;
    }
  }
  else if ( op.source.type == Operand::Type::DIRECT )
  {
    if ( (op.source.value == op.target.value)
        && (op.type == Operation::Type::MOV || op.type == Operation::Type::DIV || op.type == Operation::Type::DIF
            || op.type == Operation::Type::MOD || op.type == Operation::Type::GCD || op.type == Operation::Type::BIN) )
    {
      op.target.value++;
    }
  }

  std::pair<Operation, double> next_op;
  next_op.first = op;
  next_op.second = static_cast<double>( position_dist( gen ) ) / POSITION_RANGE;
  return next_op;
}

Program GeneratorV1::generateProgram()
{
  // use template for base program
  Program p = program_template;
  generateStateless( p, num_operations );
  applyPostprocessing( p );
  return p;
}
