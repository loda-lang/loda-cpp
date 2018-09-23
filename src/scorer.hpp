#pragma once

#include "common.hpp"

class Scorer
{
public:

  virtual ~Scorer();

  virtual number_t Score( const Sequence& s ) = 0;

};

class FixedSequenceScorer: public Scorer
{
public:

  FixedSequenceScorer( const Sequence& target );

  virtual ~FixedSequenceScorer();

  virtual number_t Score( const Sequence& s ) override;

private:
  Sequence target_;

};
