#include "program.hpp"

#include "number.hpp"

const std::array<Operation::Type, 17> Operation::Types = { Operation::Type::NOP, Operation::Type::MOV,
    Operation::Type::ADD, Operation::Type::SUB, Operation::Type::MUL, Operation::Type::DIV, Operation::Type::MOD,
    Operation::Type::POW, Operation::Type::LOG, Operation::Type::FAC, Operation::Type::GCD, Operation::Type::BIN,
    Operation::Type::CMP, Operation::Type::LPB, Operation::Type::LPE, Operation::Type::CLR, Operation::Type::DBG, };

const Operation::Metadata& Operation::Metadata::get( Type t )
{
  static Operation::Metadata nop { Operation::Type::NOP, "nop", 'n', 0, false, false, false, 0 };
  static Operation::Metadata mov { Operation::Type::MOV, "mov", 'm', 2, true, false, true, 60 };
  static Operation::Metadata add { Operation::Type::ADD, "add", 'a', 2, true, true, true, 40 };
  static Operation::Metadata sub { Operation::Type::SUB, "sub", 's', 2, true, true, true, 30 };
  static Operation::Metadata mul { Operation::Type::MUL, "mul", 'u', 2, true, true, true, 12 };
  static Operation::Metadata div { Operation::Type::DIV, "div", 'd', 2, true, true, true, 5 };
  static Operation::Metadata mod { Operation::Type::MOD, "mod", 'o', 2, true, true, true, 2 };
  static Operation::Metadata pow { Operation::Type::POW, "pow", 'p', 2, true, true, true, 4 };
  static Operation::Metadata log { Operation::Type::LOG, "log", 'k', 2, true, true, true, 1 };
  static Operation::Metadata fac { Operation::Type::FAC, "fac", 'f', 1, true, true, true, 1 };
  static Operation::Metadata gcd { Operation::Type::GCD, "gcd", 'g', 2, true, true, true, 2 };
  static Operation::Metadata bin { Operation::Type::BIN, "bin", 'b', 2, true, true, true, 1 };
  static Operation::Metadata cmp { Operation::Type::CMP, "cmp", 'c', 2, true, true, true, 1 };
  static Operation::Metadata lpb { Operation::Type::LPB, "lpb", 'l', 2, true, true, false, 15 };
  static Operation::Metadata lpe { Operation::Type::LPE, "lpe", 'e', 0, true, false, false, 0 };
  static Operation::Metadata clr { Operation::Type::CLR, "clr", 'r', 2, true, false, true, 1 };
  static Operation::Metadata dbg { Operation::Type::DBG, "dbg", 'c', 0, false, false, false, 0 };
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
  case Operation::Type::LOG:
    return log;
  case Operation::Type::FAC:
    return fac;
  case Operation::Type::GCD:
    return gcd;
  case Operation::Type::BIN:
    return bin;
  case Operation::Type::CMP:
    return cmp;
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
