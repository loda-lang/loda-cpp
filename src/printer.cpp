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

std::string GetOperand( Operand op )
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

std::string GetBinaryOperation( int indent, const std::string& name, BinaryOperation& op )
{
  return GetIndent( indent ) + name + " " + GetOperand( op.target ) + "," + GetOperand( op.source );
}

void Printer::Print( Operation::UPtr& op, std::ostream& out, int indent )
{
  switch ( op->GetType() )
  {
  case Operation::Type::NOP:
  {
    out << GetIndent( indent );
    break;
  }
  case Operation::Type::DBG:
  {
    out << GetIndent( indent ) << "dbg";
    break;
  }
  case Operation::Type::MOV:
  {
    out << GetBinaryOperation( indent, "mov", *BinaryOperation::Cast( op ) );
    break;
  }
  case Operation::Type::ADD:
  {
    out << GetBinaryOperation( indent, "add", *BinaryOperation::Cast( op ) );
    break;
  }
  case Operation::Type::SUB:
  {
    out << GetBinaryOperation( indent, "sub", *BinaryOperation::Cast( op ) );
    break;
  }
  case Operation::Type::LPB:
  {
    out << GetBinaryOperation( indent, "lpb", *BinaryOperation::Cast( op ) );
    break;
  }
  case Operation::Type::LPE:
  {
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

void Printer::Print( Program& p, std::ostream& out )
{
  int indent = 0;
  for ( auto& op : p.ops )
  {
    if ( op->GetType() == Operation::Type::LPE )
    {
      indent -= 2;
    }
    Print( op, out, indent );
    if ( op->GetType() == Operation::Type::LPB )
    {
      indent += 2;
    }
  }

}
