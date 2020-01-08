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

std::string getUnaryOperation( int indent, const std::string &name, const Operation &op )
{
  return getIndent( indent ) + name + " " + getOperand( op.target );
}

std::string getBinaryOperation( int indent, const std::string &name, const Operation &op )
{
  return getIndent( indent ) + name + " " + getOperand( op.target ) + "," + getOperand( op.source );
}

void Printer::print( const Operation &op, std::ostream &out, int indent )
{
  auto &metadata = Operation::Metadata::get( op.type );
  if ( metadata.num_operands == 0 && op.type != Operation::Type::NOP )
  {
    out << getIndent( indent ) << metadata.name;
  }
  else if ( metadata.num_operands == 1 )
  {
    out << getUnaryOperation( indent, metadata.name, op );
  }
  else if ( metadata.num_operands == 2 )
  {
    out << getBinaryOperation( indent, metadata.name, op );
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
