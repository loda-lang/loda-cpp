#include "program_util.hpp"

#include "program.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stack>

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

bool ProgramUtil::isArithmetic( Operation::Type t )
{
  // TODO: model this as metadata?
  return (t != Operation::Type::NOP && t != Operation::Type::DBG && t != Operation::Type::LPB
      && t != Operation::Type::LPE && t != Operation::Type::CLR && t != Operation::Type::CAL);
}

bool ProgramUtil::hasIndirectOperand( const Operation &op )
{
  const auto num_ops = Operation::Metadata::get( op.type ).num_operands;
  return (num_ops > 0 && op.target.type == Operand::Type::INDIRECT)
      || (num_ops > 1 && op.source.type == Operand::Type::INDIRECT);
}

bool ProgramUtil::areIndependent( const Operation& op1, const Operation& op2 )
{
  if ( !isArithmetic( op1.type ) || !isArithmetic( op2.type ) )
  {
    return false;
  }
  if ( hasIndirectOperand( op1 ) || hasIndirectOperand( op2 ) )
  {
    return false;
  }
  if ( op1.target.value == op2.target.value )
  {
    return false;
  }
  if ( op1.source.type == Operand::Type::DIRECT && op2.target.value == op1.source.value )
  {
    return false;
  }
  if ( op2.source.type == Operand::Type::DIRECT && op1.target.value == op2.source.value )
  {
    return false;
  }
  return true;
}

bool ProgramUtil::getUsedMemoryCells( const Program &p, std::unordered_set<number_t> &used_cells,
    number_t &largest_used, size_t max_memory )
{
  for ( auto &op : p.ops )
  {
    size_t region_length = 1;
    if ( op.source.type == Operand::Type::INDIRECT || op.target.type == Operand::Type::INDIRECT )
    {
      return false;
    }
    if ( op.type == Operation::Type::LPB || op.type == Operation::Type::CLR )
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
    if ( region_length > max_memory )
    {
      return false;
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
  else if ( metadata.num_operands == 1
      || (op.type == Operation::Type::LPB && op.source.type == Operand::Type::CONSTANT && op.source.value == 1) ) // lpb has an optional second argument
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

void ProgramUtil::print( const Program &p, std::ostream &out, std::string newline )
{
  int indent = 0;
  for ( auto &op : p.ops )
  {
    if ( op.type == Operation::Type::LPE )
    {
      indent -= 2;
    }
    print( op, out, indent );
    out << newline;
    if ( op.type == Operation::Type::LPB )
    {
      indent += 2;
    }
  }
}

void ProgramUtil::exportToDot( Program p, std::ostream &out )
{
  removeOps( p, Operation::Type::NOP );
  out << "digraph G {" << std::endl;
  // nodes
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    out << "  o" << i << " [label=\"" << operationToString( p.ops[i] ) << "\"];" << std::endl;
  }
  // edges
  std::stack<size_t> lpbs;
  for ( size_t i = 1; i < p.ops.size(); i++ )
  {
    out << "  o" << (i - 1) << " -> ";
    if ( p.ops[i - 1].type == Operation::Type::LPE )
    {
      out << "{ o" << i << " o" << lpbs.top() << " }" << std::endl;
      lpbs.pop();
    }
    else
    {
      out << "o" << i << std::endl;
    }
    if ( p.ops[i - 1].type == Operation::Type::LPB )
    {
      lpbs.push( i - 1 );
    }
  }
  out << "}" << std::endl;
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
