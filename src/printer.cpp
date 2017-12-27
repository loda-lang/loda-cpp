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

std::string GetArgument( Argument a, Program& p )
{
  switch ( a.type )
  {
  case Argument::Type::CONSTANT:
    return std::to_string( a.value.constant );
  case Argument::Type::VARIABLE:
    return p.var_names.at( a.value.variable );
  }
  return "";
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
//      auto cmt = Comment::Cast( op );
//      if ( !cmt->value.empty() )
//      {
//        out << GetIndent( indent ) << "# " << cmt->value << std::endl;
//      }
      break;
    }
    case Operation::Type::MOV:
    {
      auto mov = Mov::Cast( op );
      out << GetIndent( indent ) << "mov " << p.var_names.at( mov->target ) << "," << GetArgument( mov->source, p )
          << std::endl;
      break;
    }
    case Operation::Type::ADD:
    {
      auto add = Add::Cast( op );
      out << GetIndent( indent ) << "add " << p.var_names.at( add->target ) << "," << GetArgument( add->source, p )
          << std::endl;
      break;
    }
    case Operation::Type::SUB:
    {
      auto sub = Sub::Cast( op );
      out << GetIndent( indent ) << "sub " << p.var_names.at( sub->target ) << "," << GetArgument( sub->source, p )
          << std::endl;
      break;
    }
    case Operation::Type::LPB:
    {
      auto loop_begin = LoopBegin::Cast( op );
      out << GetIndent( indent ) << "lpb ";
      for ( size_t i = 0; i < loop_begin->loop_vars.size(); i++ )
      {
        if ( i > 0 ) out << ",";
        out << p.var_names.at( loop_begin->loop_vars.at( i ) );
      }
      out << std::endl;
      indent += 2;
      break;
    }
    case Operation::Type::LPE:
    {
      indent -= 2;
      out << GetIndent( indent ) << "lpe " << std::endl;
      break;
    }
    }
  }

}
