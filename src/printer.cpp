#include "printer.hpp"
#include "program.hpp"

std::string GetIndent( int indent )
{
  std::string s;
  for ( int i = 0; i < indent; i++ )
  {
    s += " ";
  }
  return s;
}

std::string GetOperand( Operand a, Program& p )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
    return std::to_string( a.value );
  case Operand::Type::MEM_ACCESS_DIRECT:
    return "$" + std::to_string( a.value );
  case Operand::Type::MEM_ACCESS_INDIRECT:
    return "$$" + std::to_string( a.value );
  }
  return "";
}

std::string GetBinaryOperation( int indent, const std::string& name, BinaryOperation& op, Program& p )
{
  return GetIndent( indent ) + name + " " + GetOperand( op.target, p ) + "," + GetOperand( op.source, p );
}

void Printer::Print( Program& p, std::ostream& out )
{
  int indent = 0;
  for ( auto& op : p.ops )
  {
    switch ( op->GetType() )
    {
    case Operation::Type::NOP:
    {
      out << GetIndent( indent );
      break;
    }
    case Operation::Type::MOV:
    {
      out << GetBinaryOperation( indent, "mov", *BinaryOperation::Cast( op ), p );
      break;
    }
    case Operation::Type::ADD:
    {
      out << GetBinaryOperation( indent, "add", *BinaryOperation::Cast( op ), p );
      break;
    }
    case Operation::Type::SUB:
    {
      out << GetBinaryOperation( indent, "sub", *BinaryOperation::Cast( op ), p );
      break;
    }
    case Operation::Type::LPB:
    {
      out << GetBinaryOperation( indent, "lpb", *BinaryOperation::Cast( op ), p );
      indent += 2;
      break;
    }
    case Operation::Type::LPE:
    {
      indent -= 2;
      out << GetIndent( indent ) << "lpe";
      break;
    }
    }
    if ( !op->comment.empty() )
    {
      out << " ; " << op->comment;
    }
    out << std::endl;
  }

}
