#pragma once

#include "number.hpp"

class Scorer
{
public:

  virtual ~Scorer();

  virtual number_t score( const Sequence& s ) = 0;

};

class FixedSequenceScorer: public Scorer
{
public:

  FixedSequenceScorer( const Sequence& target );

  virtual ~FixedSequenceScorer();

  virtual number_t score( const Sequence& s ) override;

private:
  Sequence target_;

};
