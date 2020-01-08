#include "program.hpp"

const std::array<Operation::Type, 14> Operation::Types = { Operation::Type::NOP, Operation::Type::MOV,
    Operation::Type::ADD, Operation::Type::SUB, Operation::Type::MUL, Operation::Type::DIV, Operation::Type::MOD,
    Operation::Type::POW, Operation::Type::FAC, Operation::Type::GCD, Operation::Type::LPB, Operation::Type::LPE,
	Operation::Type::CLR, Operation::Type::DBG, };

const Operation::Metadata& Operation::Metadata::get( Type t )
{
  static Operation::Metadata nop { Operation::Type::NOP, "nop", 'n', 0, false, false };
  static Operation::Metadata mov { Operation::Type::MOV, "mov", 'm', 2, true, true };
  static Operation::Metadata add { Operation::Type::ADD, "add", 'a', 2, true, true };
  static Operation::Metadata sub { Operation::Type::SUB, "sub", 's', 2, true, true };
  static Operation::Metadata mul { Operation::Type::MUL, "mul", 'u', 2, true, true };
  static Operation::Metadata div { Operation::Type::DIV, "div", 'd', 2, true, true };
  static Operation::Metadata mod { Operation::Type::MOD, "mod", 'o', 2, true, true };
  static Operation::Metadata pow { Operation::Type::POW, "pow", 'p', 2, true, true };
  static Operation::Metadata fac { Operation::Type::FAC, "fac", 'f', 1, true, true };
  static Operation::Metadata gcd { Operation::Type::GCD, "gcd", 'g', 2, true, true };
  static Operation::Metadata lpb { Operation::Type::LPB, "lpb", 'l', 2, true, false };
  static Operation::Metadata lpe { Operation::Type::LPE, "lpe", 'e', 0, true, false };
  static Operation::Metadata clr { Operation::Type::CLR, "clr", 'c', 0, false, true };
  static Operation::Metadata dbg { Operation::Type::DBG, "dbg", 'b', 0, false, false };
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
  case Operation::Type::MOD:
    return mod;
  case Operation::Type::POW:
    return pow;
  case Operation::Type::FAC:
    return fac;
  case Operation::Type::GCD:
    return gcd;
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
