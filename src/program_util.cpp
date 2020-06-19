#include "program_util.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

void ProgramUtil::removeOps( Program &p, Operation::Type type )
{
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == type )
    {
      it = p.ops.erase( it );
    }
    else
    {
      it++;
    }
  }
}

size_t ProgramUtil::numOps( const Program &p, bool withNops )
{
  if ( withNops )
  {
    return p.ops.size();
  }
  else
  {
    size_t num_ops = 0;
    for ( auto &op : p.ops )
    {
      if ( op.type != Operation::Type::NOP )
      {
        num_ops++;
      }
    }
    return num_ops;
  }
}

size_t ProgramUtil::numOps( const Program &p, Operand::Type type )
{
  size_t num_ops = 0;
  for ( auto &op : p.ops )
  {
    auto &m = Operation::Metadata::get( op.type );
    if ( m.num_operands == 1 && op.target.type == type )
    {
      num_ops++;
    }
    else if ( m.num_operands == 2 && (op.source.type == type || op.target.type == type) )
    {
      num_ops++;
    }
  }
  return num_ops;
}

std::string getIndent( int indent )
{
  std::string s;
  for ( int i = 0; i < indent; i++ )
  {
    s += " ";
  }
  return s;
}

std::string getOperand( Operand op )
{
  switch ( op.type )
  {
  case Operand::Type::CONSTANT:
    return std::to_string( op.value );
  case Operand::Type::DIRECT:
    return "$" + std::to_string( op.value );
  case Operand::Type::INDIRECT:
    return "$$" + std::to_string( op.value );
  }
  return "";
}

void ProgramUtil::print( const Operation &op, std::ostream &out, int indent )
{
  auto &metadata = Operation::Metadata::get( op.type );
  if ( metadata.num_operands == 0 && op.type != Operation::Type::NOP )
  {
    out << getIndent( indent ) << metadata.name;
  }
  else if ( metadata.num_operands == 1 )
  {
    out << getIndent( indent ) << metadata.name << " " << getOperand( op.target );
  }
  else if ( metadata.num_operands == 2 )
  {
    out << getIndent( indent ) << metadata.name << " " << getOperand( op.target ) << "," << getOperand( op.source );
  }
  if ( !op.comment.empty() )
  {
    out << " ; " << op.comment;
  }
}

void ProgramUtil::print( const Program &p, std::ostream &out )
{
  int indent = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type == Operation::Type::LPE )
    {
      indent -= 2;
    }
    print( op, out, indent );
    out << std::endl;
    if ( op.type == Operation::Type::LPB )
    {
      indent += 2;
    }
  }
}

size_t ProgramUtil::hash( const Program &p )
{
  size_t h = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type != Operation::Type::NOP )
    {
      h = (h * 3) + hash( op );
    }
  }
  return h;
}

size_t ProgramUtil::hash( const Operation &op )
{
  auto &meta = Operation::Metadata::get( op.type );
  size_t h = static_cast<size_t>( op.type );
  if ( meta.num_operands > 0 )
  {
    h = (5 * h) + hash( op.target );
  }
  if ( meta.num_operands > 1 )
  {
    h = (7 * h) + hash( op.source );
  }
  return h;
}

size_t ProgramUtil::hash( const Operand &op )
{
  return (11 * static_cast<size_t>( op.type )) + op.value;
}

ProgramUtil::Stats::Stats()
    :
    num_programs( 0 ),
    num_ops_per_type( Operation::Types.size(), 0 )
{
}

void ProgramUtil::Stats::load( const std::string &path )
{
  const std::string sep( "," );
  std::ifstream constants( path + "/constant_counts.csv" );
  std::string line, key, value;
  while ( std::getline( constants, line ) )
  {
    std::stringstream s( line );
    std::getline( s, key, ',' );
    std::getline( s, value );
    num_constants[std::stol( key )] = std::stol( value );
  }
  constants.close();
  std::ifstream op_counts( path + "/operation_counts.csv" );

  while ( std::getline( op_counts, line ) )
  {
    std::stringstream s( line );
    std::getline( s, key, ',' );
    std::getline( s, value );
    auto type = Operation::Metadata::get( key ).type;
    num_ops_per_type.at( static_cast<size_t>( type ) ) = std::stol( value );
  }
  op_counts.close();
  // TODO: remaining stats
}

void ProgramUtil::Stats::save( const std::string &path )
{
  const std::string sep( "," );
  std::ofstream constants( path + "/constant_counts.csv" );
  for ( auto &e : num_constants )
  {
    constants << std::to_string( e.first ) << sep << std::to_string( e.second ) << "\n";
  }
  constants.close();
  std::ofstream lengths( path + "/program_lengths.csv" );
  for ( size_t i = 0; i < num_programs_per_length.size(); i++ )
  {
    if ( num_programs_per_length[i] > 0 )
    {
      lengths << std::to_string( i ) << sep << std::to_string( num_programs_per_length[i] ) << "\n";
    }
  }
  lengths.close();
  std::ofstream op_counts( path + "/operation_counts.csv" );
  for ( size_t i = 0; i < num_ops_per_type.size(); i++ )
  {
    if ( num_ops_per_type[i] > 0 )
    {
      op_counts << Operation::Metadata::get( static_cast<Operation::Type>( i ) ).name << sep
          << std::to_string( num_ops_per_type[i] ) << "\n";
    }
  }
  op_counts.close();
}

void ProgramUtil::Stats::update( const Program &program )
{
  num_programs++;
  const size_t num_ops = ProgramUtil::numOps( program, false );
  if ( num_ops >= num_programs_per_length.size() )
  {
    num_programs_per_length.resize( num_ops + 1 );
  }
  num_programs_per_length[num_ops]++;
  for ( auto &op : program.ops )
  {
    num_ops_per_type[static_cast<size_t>( op.type )]++;
    if ( Operation::Metadata::get( op.type ).num_operands == 2 && op.source.type == Operand::Type::CONSTANT )
    {
      if ( num_constants.find( op.source.value ) == num_constants.end() )
      {
        num_constants[op.source.value] = 0;
      }
      num_constants[op.source.value]++;
    }
  }
}
