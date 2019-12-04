#include "printer.hpp"
#include "program.hpp"

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
  case Operand::Type::MEM_ACCESS_DIRECT:
    return "$" + std::to_string( op.value );
  case Operand::Type::MEM_ACCESS_INDIRECT:
    return "$$" + std::to_string( op.value );
  }
  return "";
}

std::string getBinaryOperation( int indent, const std::string &name, const Operation &op )
{
  return getIndent( indent ) + name + " " + getOperand( op.target ) + "," + getOperand( op.source );
}

void Printer::print( const Operation &op, std::ostream &out, int indent )
{
  switch ( op.type )
  {
  case Operation::Type::NOP:
  {
    out << getIndent( indent );
    break;
  }
  case Operation::Type::MOV:
  {
    out << getBinaryOperation( indent, "mov", op );
    break;
  }
  case Operation::Type::ADD:
  {
    out << getBinaryOperation( indent, "add", op );
    break;
  }
  case Operation::Type::SUB:
  {
    out << getBinaryOperation( indent, "sub", op );
    break;
  }
  case Operation::Type::MUL:
  {
    out << getBinaryOperation( indent, "mul", op );
    break;
  }
  case Operation::Type::DIV:
  {
    out << getBinaryOperation( indent, "div", op );
    break;
  }
  case Operation::Type::LPB:
  {
    out << getBinaryOperation( indent, "lpb", op );
    break;
  }
  case Operation::Type::LPE:
  {
    out << getIndent( indent ) << "lpe";
    break;
  }
  case Operation::Type::CLR:
  {
    out << getIndent( indent ) << "clr";
    break;
  }
  case Operation::Type::DBG:
  {
    out << getIndent( indent ) << "dbg";
    break;
  }
  }
  if ( !op.comment.empty() )
  {
    out << " ; " << op.comment;
  }
}

void Printer::print( const Program &p, std::ostream &out )
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
