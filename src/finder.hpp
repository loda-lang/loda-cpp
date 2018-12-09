#pragma once

#include "generator.hpp"
#include "number.hpp"

inline bool less_than_score( const Generator::UPtr& g1, const Generator::UPtr& g2 )
{
  return (g1->score < g2->score);
}

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

class Finder
{
public:

  Finder( const Settings& settings );

  Program find( Scorer& scorer, size_t seed, size_t max_iterations );

private:
  const Settings& settings;

};
