#pragma once

#include "number.hpp"
#include "program.hpp"

class Synthesizer
{
public:

  virtual ~Synthesizer()
  {
  }

  virtual bool synthesize( const Sequence &seq, Program& program ) = 0;

};

class PolynomialSynthesizer: public Synthesizer
{
public:

  virtual ~PolynomialSynthesizer()
  {
  }

  virtual bool synthesize( const Sequence &seq, Program& program ) override;

};
