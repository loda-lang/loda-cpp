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

void Printer::Print( Program& p, std::ostream& out )
{
  int indent = 0;
  for ( auto& op : p.ops )
  {
    switch ( op->GetType() )
    {
    case Operation::Type::CMT:
    {
      auto cmt = Comment::Cast( op );
      if ( !cmt->value.empty() )
      {
        out << GetIndent( indent ) << "# " << cmt->value << std::endl;
      }
      break;
    }
    case Operation::Type::SET:
    {
      auto set = Set::Cast( op );
      out << GetIndent( indent ) << "set " << p.var_names.at( set->target_var ) << "," << (int) set->value << std::endl;
      break;
    }
    case Operation::Type::CPY:
    {
      auto copy = Copy::Cast( op );
      out << GetIndent( indent ) << "cpy " << p.var_names.at( copy->target_var ) << ","
          << p.var_names.at( copy->source_var ) << std::endl;
      break;
    }
    case Operation::Type::ADD:
    {
      auto add = Add::Cast( op );
      out << GetIndent( indent ) << "add " << p.var_names.at( add->target_var ) << ","
          << p.var_names.at( add->source_var ) << std::endl;
      break;
    }
    case Operation::Type::SUB:
    {
      auto sub = Sub::Cast( op );
      out << GetIndent( indent ) << "sub " << p.var_names.at( sub->target_var ) << ","
          << p.var_names.at( sub->source_var ) << std::endl;
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
