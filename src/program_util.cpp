#include "program_util.hpp"

#include "program.hpp"

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

bool ProgramUtil::replaceOps( Program &p, Operation::Type oldType, Operation::Type newType )
{
  bool result = false;
  for ( Operation &op : p.ops )
  {
    if ( op.type == oldType )
    {
      op.type = newType;
      result = true;
    }
  }
  return result;
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

size_t ProgramUtil::numOps( const Program &p, Operation::Type type )
{
  size_t num_ops = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type == type )
    {
      num_ops++;
    }
  }
  return num_ops;
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

std::string ProgramUtil::operandToString( const Operand &op )
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

std::string ProgramUtil::operationToString( const Operation &op )
{
  auto &metadata = Operation::Metadata::get( op.type );
  std::string str;
  if ( metadata.num_operands == 0 && op.type != Operation::Type::NOP )
  {
    str = metadata.name;
  }
  else if ( metadata.num_operands == 1 )
  {
    str = metadata.name + " " + operandToString( op.target );
  }
  else if ( metadata.num_operands == 2 )
  {
    str = metadata.name + " " + operandToString( op.target ) + "," + operandToString( op.source );
  }
  if ( !op.comment.empty() )
  {
    str = str + " ; " + op.comment;
  }
  return str;
}

void ProgramUtil::print( const Operation &op, std::ostream &out, int indent )
{
  out << getIndent( indent ) << operationToString( op );
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
