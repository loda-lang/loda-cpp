#include "program.hpp"

const std::array<Operation::Type, 10> Operation::Types = { Operation::Type::NOP, Operation::Type::MOV,
    Operation::Type::ADD, Operation::Type::SUB, Operation::Type::MUL, Operation::Type::DIV, Operation::Type::LPB,
    Operation::Type::LPE, Operation::Type::CLR, Operation::Type::DBG, };

const Operation::Metadata& Operation::Metadata::get( Type t )
{
  static Operation::Metadata nop { Operation::Type::NOP, "", 0, 0 };
  static Operation::Metadata mov { Operation::Type::MOV, "mov", 'm', 2 };
  static Operation::Metadata add { Operation::Type::ADD, "add", 'a', 2 };
  static Operation::Metadata sub { Operation::Type::SUB, "sub", 's', 2 };
  static Operation::Metadata mul { Operation::Type::MUL, "mul", 'u', 2 };
  static Operation::Metadata div { Operation::Type::DIV, "div", 'd', 2 };
  static Operation::Metadata lpb { Operation::Type::LPB, "lpb", 'l', 2 };
  static Operation::Metadata lpe { Operation::Type::LPE, "lpe", 'e', 0 };
  static Operation::Metadata clr { Operation::Type::CLR, "clr", 'c', 0 };
  static Operation::Metadata dbg { Operation::Type::DBG, "dbg", 'b', 0 };
  switch ( t )
  {
  case Operation::Type::NOP:
    return nop;
  case Operation::Type::MOV:
    return mov;
  case Operation::Type::ADD:
    return add;
  case Operation::Type::SUB:
    return sub;
  case Operation::Type::MUL:
    return mul;
  case Operation::Type::DIV:
    return div;
  case Operation::Type::LPB:
    return lpb;
  case Operation::Type::LPE:
    return lpe;
  case Operation::Type::CLR:
    return clr;
  case Operation::Type::DBG:
    return dbg;
  }
  return nop;
}

void Program::removeOps( Operation::Type type )
{
  auto it = ops.begin();
  while ( it != ops.end() )
  {
    if ( it->type == type )
    {
      it = ops.erase( it );
    }
    else
    {
      it++;
    }
  }
}

size_t Program::num_ops( bool withNops ) const
{
  if ( withNops )
  {
    return ops.size();
  }
  else
  {
    size_t num_ops = 0;
    for ( auto &op : ops )
    {
      if ( op.type != Operation::Type::NOP )
      {
        num_ops++;
      }
    }
    return num_ops;
  }
}

size_t Program::num_ops( Operand::Type type ) const
{
  size_t num_ops = 0;
  for ( auto &op : ops )
  {
    if ( op.source.type == type || op.target.type == type )
    {
      num_ops++;
    }
  }
  return num_ops;
}

bool Program::operator==( const Program &p ) const
{
  if ( p.ops.size() != ops.size() )
  {
    return false;
  }
  for ( size_t i = 0; i < ops.size(); ++i )
  {
    if ( !(ops[i] == p.ops[i]) )
    {
      return false;
    }
  }
  return true;
}
