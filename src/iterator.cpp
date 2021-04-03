#include "iterator.hpp"

#include "program_util.hpp"

#include <iostream>

const Operand Iterator::SMALLEST_SOURCE( Operand::Type::CONSTANT, 0 );
const Operand Iterator::SMALLEST_TARGET( Operand::Type::DIRECT, 0 );
const Operation Iterator::SMALLEST_OPERATION( Operation::Type::MOV, SMALLEST_TARGET, SMALLEST_SOURCE );

bool Iterator::inc( Operand &o )
{
  if ( o.value * 2 < size_ )
  {
    o.value++;
    return true;
  }
  switch ( o.type )
  {
  case Operand::Type::CONSTANT:
    o = Operand( Operand::Type::DIRECT, 0 );
    return true;
  case Operand::Type::DIRECT:
  case Operand::Type::INDIRECT: // we exclude indirect memory access
    return false;
  }
  return false;
}

bool Iterator::inc( Operation &op )
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
  op.source = SMALLEST_SOURCE;

  // try to increase target operand
  if ( inc( op.target ) )
  {
    return true;
  }
  op.target = SMALLEST_TARGET;

  // try to increase type
  switch ( op.type )
  {
  case Operation::Type::NOP: // excluded
  case Operation::Type::DBG: // excluded
  case Operation::Type::CLR: // excluded
  case Operation::Type::CAL: // excluded
  case Operation::Type::POW: // excluded
  case Operation::Type::LOG: // excluded
  case Operation::Type::MIN: // excluded
  case Operation::Type::MAX: // excluded

  case Operation::Type::MOV:
    op.type = Operation::Type::ADD;
    return true;

  case Operation::Type::ADD:
    op.type = Operation::Type::SUB;
    return true;

  case Operation::Type::SUB:
    op.type = Operation::Type::TRN;
    return true;

  case Operation::Type::TRN:
    op.type = Operation::Type::MUL;
    return true;

  case Operation::Type::MUL:
    op.type = Operation::Type::DIV;
    return true;

  case Operation::Type::DIV:
    op.type = Operation::Type::DIF;
    return true;

  case Operation::Type::DIF:
    op.type = Operation::Type::MOD;
    return true;

  case Operation::Type::MOD:
    op.type = Operation::Type::GCD;
    return true;

  case Operation::Type::GCD:
    op.type = Operation::Type::BIN;
    return true;

  case Operation::Type::BIN:
    op.type = Operation::Type::CMP;
    return true;

  case Operation::Type::CMP:
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
  for ( auto op = p_.ops.rbegin(); op != p_.ops.rend(); ++op )
  {
    if ( inc( *op ) )
    {
      while ( ProgramUtil::isNop( *op ) )
      {
        inc( *op );
      }
      return p_;
    }
    else
    {
      *op = SMALLEST_OPERATION;
    }
  }
  p_.ops.insert( p_.ops.begin(), SMALLEST_OPERATION );
  size_ = p_.ops.size();
//  int open_loops = 0;
//  for ( const auto& op : p_.ops )
//  {
//    if ( op.type == Operation::Type::LPB )
//    {
//      open_loops++;
//    }
//    else if ( op.type == Operation::Type::LPE )
//    {
//      open_loops--;
//    }
//  }
  return p_;
}
