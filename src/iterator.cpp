#include "iterator.hpp"

#include "program_util.hpp"

#include <iostream>

const Operand Iterator::CONSTANT_ZERO( Operand::Type::CONSTANT, 0 );
const Operand Iterator::CONSTANT_ONE( Operand::Type::CONSTANT, 1 );
const Operand Iterator::DIRECT_ZERO( Operand::Type::DIRECT, 0 );
const Operand Iterator::DIRECT_ONE( Operand::Type::DIRECT, 1 );

const Operand Iterator::SMALLEST_SOURCE = CONSTANT_ZERO;
const Operand Iterator::SMALLEST_TARGET = DIRECT_ZERO;
const Operation Iterator::SMALLEST_OPERATION( Operation::Type::MOV, DIRECT_ONE, CONSTANT_ZERO ); // never override $0

bool Iterator::inc( Operand &o )
{
  if ( o.value * 4 < size_ )
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
    op.type = Operation::Type::POW;
    return true;

  case Operation::Type::POW:
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

bool Iterator::shouldSkip( const Operation& op )
{
  if ( ProgramUtil::isNop( op ) )
  {
    return true;
  }
  // check for trivial operations that can be expressed in a simpler way
  if ( op.target == op.source
      && (op.type == Operation::Type::ADD || op.type == Operation::Type::SUB || op.type == Operation::Type::TRN
          || op.type == Operation::Type::MUL || op.type == Operation::Type::DIV || op.type == Operation::Type::DIF
          || op.type == Operation::Type::MOD || op.type == Operation::Type::GCD || op.type == Operation::Type::BIN
          || op.type == Operation::Type::CMP) )
  {
    return true;
  }
  if ( op.source == CONSTANT_ZERO
      && (op.type == Operation::Type::MUL || op.type == Operation::Type::DIV || op.type == Operation::Type::DIF
          || op.type == Operation::Type::MOD || op.type == Operation::Type::POW || op.type == Operation::Type::GCD
          || op.type == Operation::Type::BIN) )
  {
    return true;
  }
  if ( op.source == CONSTANT_ONE
      && (op.type == Operation::Type::MOD || op.type == Operation::Type::POW || op.type == Operation::Type::GCD
          || op.type == Operation::Type::BIN) )
  {
    return true;
  }

  return false;
}

Program Iterator::next()
{
  for ( auto op = p_.ops.rbegin(); op != p_.ops.rend(); ++op )
  {
    if ( inc( *op ) )
    {
      while ( shouldSkip( *op ) )
      {
        inc( *op );
      }
      return getFixed();
    }
    else
    {
      *op = SMALLEST_OPERATION;
    }
  }
  p_.ops.insert( p_.ops.begin(), SMALLEST_OPERATION );
  size_ = p_.ops.size();
  return getFixed();
}

Program Iterator::getFixed() const
{
  int64_t open_loops = 0;
  Program p = p_;
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == Operation::Type::LPB )
    {
      open_loops++;
    }
    else if ( it->type == Operation::Type::LPE )
    {
      if ( open_loops > 0 )
      {
        open_loops--;
      }
      else
      {
        it = p.ops.erase( it );
        continue;
      }
    }
    it++;
  }
  while ( open_loops > 0 )
  {
    p.ops.push_back( Operation( Operation::Type::LPE ) );
    open_loops--;
  }
  return p;
}
