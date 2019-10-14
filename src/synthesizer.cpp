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

  // constant sequence
  if ( slope == 0 )
  {
    program.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, offset ) ) );
    return true;
  }

  // non-constant (linear) sequence
  program.ops.push_back(
      Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
          Operand( Operand::Type::CONSTANT, offset ) ) );
  program.ops.push_back(
      Operation( Operation::Type::LPB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ),
          Operand( Operand::Type::CONSTANT, 1 ) ) );
  if ( slope > 0 )
  {
    program.ops.push_back(
        Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, slope ) ) );
  }
  else
  {
    program.ops.push_back(
        Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, -slope ) ) );
  }
  program.ops.push_back(
      Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ),
          Operand( Operand::Type::CONSTANT, 1 ) ) );
  program.ops.push_back( Operation( Operation::Type::LPE ) );
  return true;
}
