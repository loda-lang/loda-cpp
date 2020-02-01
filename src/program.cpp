#include "program.hpp"

#include "number.hpp"

const std::array<Operation::Type, 14> Operation::Types = { Operation::Type::NOP, Operation::Type::MOV,
    Operation::Type::ADD, Operation::Type::SUB, Operation::Type::MUL, Operation::Type::DIV, Operation::Type::MOD,
    Operation::Type::POW, Operation::Type::FAC, Operation::Type::GCD, Operation::Type::LPB, Operation::Type::LPE,
    Operation::Type::CLR, Operation::Type::DBG, };

const Operation::Metadata& Operation::Metadata::get( Type t )
{
  static Operation::Metadata nop { Operation::Type::NOP, "nop", 'n', 0, false, false, 0 };
  static Operation::Metadata mov { Operation::Type::MOV, "mov", 'm', 2, true, true, 24 };
  static Operation::Metadata add { Operation::Type::ADD, "add", 'a', 2, true, true, 24 };
  static Operation::Metadata sub { Operation::Type::SUB, "sub", 's', 2, true, true, 24 };
  static Operation::Metadata mul { Operation::Type::MUL, "mul", 'u', 2, true, true, 24 };
  static Operation::Metadata div { Operation::Type::DIV, "div", 'd', 2, true, true, 24 };
  static Operation::Metadata mod { Operation::Type::MOD, "mod", 'o', 2, true, true, 12 };
  static Operation::Metadata pow { Operation::Type::POW, "pow", 'p', 2, true, true, 18 };
  static Operation::Metadata fac { Operation::Type::FAC, "fac", 'f', 1, true, true, 6 };
  static Operation::Metadata gcd { Operation::Type::GCD, "gcd", 'g', 2, true, true, 6 };
  static Operation::Metadata lpb { Operation::Type::LPB, "lpb", 'l', 2, true, false, 8 };
  static Operation::Metadata lpe { Operation::Type::LPE, "lpe", 'e', 0, true, false, 0 };
  static Operation::Metadata clr { Operation::Type::CLR, "clr", 'c', 0, false, true, 0 };
  static Operation::Metadata dbg { Operation::Type::DBG, "dbg", 'b', 0, false, false, 0 };
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

void Program::push_front( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv )
{
  ops.insert( ops.begin(), Operation( t, Operand( tt, tv ), Operand( st, sv ) ) );
}

void Program::push_back( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv )
{
  ops.insert( ops.end(), Operation( t, Operand( tt, tv ), Operand( st, sv ) ) );
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
