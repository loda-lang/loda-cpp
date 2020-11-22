#include "synthesizer.hpp"

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
    program.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        offset );
  }
  else if ( slope > 0 )
  {
    program.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
        Program::INPUT_CELL );
    program.push_back( Operation::Type::MUL, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        slope );
    program.push_back( Operation::Type::ADD, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        offset );
  }
  else if ( slope < 0 )
  {
    return false;
  }
  return true;
}

bool PeriodicSynthesizer::synthesize( const Sequence &seq, Program &program )
{
  if ( seq.size() < 12 )
  {
    return false;
  }
  bool ok = false;
  size_t period = 0;
  const size_t max_period = seq.size() / 4;
  for ( period = 1; period <= max_period; period++ )
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
  if ( period == 1 )
  {
    program.push_back( Operation::Type::MOV, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::CONSTANT,
        seq[0] );
  }
  else
  {
    program.push_back( Operation::Type::MOD, Operand::Type::DIRECT, Program::INPUT_CELL, Operand::Type::CONSTANT,
        period );
    for ( size_t i = 0; i < period; i++ )
    {
      if ( seq[i] > 0 )
      {
        program.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 2, Operand::Type::DIRECT, Program::INPUT_CELL );
        program.push_back( Operation::Type::CMP, Operand::Type::DIRECT, 2, Operand::Type::CONSTANT, i );
        program.push_back( Operation::Type::MUL, Operand::Type::DIRECT, 2, Operand::Type::CONSTANT, seq[i] );
        program.push_back( Operation::Type::ADD, Operand::Type::DIRECT, Program::OUTPUT_CELL, Operand::Type::DIRECT,
            2 );
      }
    }
  }
  return true;
}
