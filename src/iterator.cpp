#include "iterator.hpp"

inline bool inc( Operation& op )
{
  // try to increase source operand

  // try to increase target operand

  // try to increase type
  switch ( op.type )
  {
  case Operation::Type::NOP:
  case Operation::Type::DBG:
  case Operation::Type::MOV:
    op = Operation( Operation::Type::ADD );
    return true;

  case Operation::Type::ADD:
    op = Operation( Operation::Type::SUB );
    return true;

  case Operation::Type::SUB:
    op = Operation( Operation::Type::LPB, { Operand::Type::MEM_ACCESS_DIRECT, 0 }, { } );
    return true;

  case Operation::Type::LPB:
    op = Operation( Operation::Type::LPB );
    return true;

  case Operation::Type::LPE:
    return false;
  }
}

Program Iterator::next()
{
  Operation zero( Operation::Type::ADD );
  for ( auto& op : p_.ops )
  {
    if ( inc( op ) )
    {
      return p_;
    }
    else
    {
      op = zero;
    }
  }
  p_.ops.push_back( zero );
  return p_;
}
