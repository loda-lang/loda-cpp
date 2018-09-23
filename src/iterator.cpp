#include "iterator.hpp"

inline Operand smallest_source()
{
  return Operand( Operand::Type::CONSTANT, 1 );
}

inline Operand smallest_target()
{
  return Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 );
}

bool Iterator::inc( Operand& o )
{
  if ( o.value * 4 < size_ )
  {
    o.value++;
    return true;
  }
  switch ( o.type )
  {
  case Operand::Type::CONSTANT:
    o = Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 );
    return true;
  case Operand::Type::MEM_ACCESS_DIRECT:
    o = Operand( Operand::Type::MEM_ACCESS_INDIRECT, 0 );
    return true;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    return false;
  }
  return false;
}

bool Iterator::inc( Operation& op )
{
  if ( op.type == Operation::Type::LPE )
  {
    return false;
  }

  // try to increase source operand
  if ( inc( op.source ) )
  {
    return true;
  }
  op.source = smallest_source();

  // try to increase target operand
  if ( inc( op.target ) )
  {
    return true;
  }
  op.target = smallest_target();

  // try to increase type
  switch ( op.type )
  {
  case Operation::Type::NOP:
  case Operation::Type::MOV:
  case Operation::Type::DBG:
  case Operation::Type::CLR:
    op.type = Operation::Type::ADD;
    return true;

  case Operation::Type::ADD:
    op.type = Operation::Type::SUB;
    return true;

  case Operation::Type::SUB:
    op.type = Operation::Type::LPB;
    return true;

  case Operation::Type::LPB:
    op = Operation( Operation::Type::LPE );
    return true;

  case Operation::Type::LPE:
    return false;
  }
  return false;
}

Program Iterator::next()
{
  Operation zero( Operation::Type::ADD, smallest_target(), smallest_source() );
  for ( auto op = p_.ops.rbegin(); op != p_.ops.rend(); ++op )
  {
    if ( inc( *op ) )
    {
      return p_;
    }
    else
    {
      *op = zero;
    }
  }
  p_.ops.insert( p_.ops.begin(), zero );
  size_ = p_.ops.size();

/*  int open_loops = 0;
  for ( const auto& op : p_.ops )
  {
    if ( op.type == Operation::Type::LPB )
    {
      open_loops++;
    }
    else if ( op.type == Operation::Type::LPE )
    {
      open_loops--;
    }
  }
  */
  return p_;
}
