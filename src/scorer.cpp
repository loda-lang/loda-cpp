#include "scorer.hpp"

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

number_t FixedSequenceScorer::Score( const Sequence& s )
{
  number_t score = 0;
  auto length = target_.Length();
  for ( number_t i = 0; i < length; i++ )
  {
    auto v1 = s.Get( i );
    auto v2 = target_.Get( i );
    score += (v1 > v2) ? (v1 - v2) : (v2 - v1);
  }
  return score;
}
