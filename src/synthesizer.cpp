#include "synthesizer.hpp"

#include "printer.hpp"

#include <vector>
#include <stdlib.h>

bool LinearSynthesizer::synthesize( const Sequence &seq, Program &program )
{
  if ( seq.size() < 3 || !seq.is_linear() )
  {
    return false;
  }
  int64_t offset = seq[0];
  int64_t slope = (int64_t) seq[1] - (int64_t) seq[0];

  program.ops.clear();

  if ( slope == 0 )
  {
    program.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, offset ) ) );
  }
  else if ( slope > 0 )
  {
    program.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ) ) );
    program.ops.push_back(
        Operation( Operation::Type::MUL, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, slope ) ) );
    program.ops.push_back(
        Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, offset ) ) );
  }
  else if ( slope < 0 )
  {
    return false;
  }
  return true;
}

bool PeriodicSynthesizer::synthesize( const Sequence &seq, Program &program )
{
  if ( seq.size() < 4 )
  {
    return false;
  }
  bool ok = false;
  size_t period = 0;
  const size_t max_period = seq.size() / 2;
  for ( period = 2; period < max_period; period++ )
  {
    ok = true;
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      if ( seq[i] != seq[i % period] )
      {
        ok = false;
        break;
      }
    }
    if ( ok )
    {
      break;
    }
  }
  if ( !ok )
  {
    return false;
  }
  program.ops.clear();
  program.ops.push_back(
      Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, period + 1 ),
          Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ) ) );
  program.ops.push_back(
      Operation( Operation::Type::LPB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 2 ),
          Operand( Operand::Type::MEM_ACCESS_DIRECT, period + 1 ) ) );
  program.ops.push_back(
      Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, period + 1 ),
          Operand( Operand::Type::CONSTANT, period ) ) );
  program.ops.push_back( Operation( Operation::Type::LPE ) );
  program.ops.push_back(
      Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 2 ),
          Operand( Operand::Type::MEM_ACCESS_DIRECT, period + 1 ) ) );
  program.ops.push_back(
      Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 2 ),
          Operand( Operand::Type::CONSTANT, 3 ) ) );
  for ( size_t i = 0; i < period; i++ )
  {
    program.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 3 + i ),
            Operand( Operand::Type::CONSTANT, seq[i] ) ) );
  }
  program.ops.push_back(
      Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
          Operand( Operand::Type::MEM_ACCESS_INDIRECT, 2 ) ) );
  return true;
}
