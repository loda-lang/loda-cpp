#pragma once

#include "program.hpp"
#include "sequence.hpp"

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

class PeriodicSynthesizer: public Synthesizer
{
public:

  virtual ~PeriodicSynthesizer()
  {
  }

  virtual bool synthesize( const Sequence &seq, Program &program ) override;

};
