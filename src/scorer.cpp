#include "scorer.hpp"

#include "common.hpp"

Scorer::~Scorer()
{
}

FixedSequenceScorer::FixedSequenceScorer( const Sequence& target )
    : target_( target )
{
}

FixedSequenceScorer::~FixedSequenceScorer()
{
}

number_t FixedSequenceScorer::score( const Sequence& s )
{
  number_t score = 0;
  auto size = target_.size();
  for ( number_t i = 0; i < size; i++ )
  {
    auto v1 = s.at( i );
    auto v2 = target_.at( i );
    score += (v1 > v2) ? (v1 - v2) : (v2 - v1);
  }
  return score;
}
