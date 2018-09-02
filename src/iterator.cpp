#include "iterator.hpp"

inline bool inc( Operation& op )
{
  // TODO
  return false;
}

inline void reset( Operation& op )
{
  op = Operation( Operation::Type::ADD, { Operand::Type::CONSTANT, 0 }, { Operand::Type::CONSTANT, 0 } );
}

Program Iterator::next()
{
  for ( auto& op : p_.ops )
  {
    if ( inc( op ) )
    {
      return p_;
    }
    else
    {
      reset( op );
    }
  }
  p_.ops.push_back( Operation() );
  reset( p_.ops.back() );
  return p_;
}
