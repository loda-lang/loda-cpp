#pragma once

#include "number.hpp"
#include "program.hpp"

class Synthesizer
{
public:

  virtual ~Synthesizer()
  {
  }

  virtual bool synthesize( const Sequence &seq, Program &program ) = 0;

};

class LinearSynthesizer: public Synthesizer
{
public:

  virtual ~LinearSynthesizer()
  {
  }

  virtual bool synthesize( const Sequence &seq, Program &program ) override;

};
