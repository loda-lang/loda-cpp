#pragma once

#include "sequence.hpp"

class Scorer
{
public:

  virtual ~Scorer();

  virtual Value Score( const Sequence& s ) = 0;

};

class FixedSequenceScorer: public Scorer
{
public:

  FixedSequenceScorer( const Sequence& target );

  virtual ~FixedSequenceScorer();

  virtual Value Score( const Sequence& s ) override;

private:
  Sequence target_;

};
