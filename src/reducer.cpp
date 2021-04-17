#include "reducer.hpp"

#include "semantics.hpp"
#include "util.hpp"

number_t Reducer::truncate( Sequence &seq )
{
  auto offset = seq.min( false );
  if ( offset == 0 || offset == NUM_INF )
  {
    return 0;
  }
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    seq[i] = seq[i] - offset;
  }
  return offset;
}

number_t Reducer::shrink( Sequence &seq )
{
  number_t factor = NUM_INF;
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    if ( seq[i] != 0 )
    {
      if ( factor == NUM_INF )
      {
        factor = std::abs( seq[i] );
      }
      else
      {
        factor = Semantics::gcd( factor, std::abs( seq[i] ) );
      }
    }
  }
  if ( factor == NUM_INF )
  {
    factor = 1;
  }
  if ( factor != 1 )
  {
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      seq[i] = seq[i] / factor;
    }
  }
  return factor;
}

Sequence subPoly( const Sequence &s, number_t factor, int64_t exp )
{
  Sequence t;
  t.resize( s.size() );
  for ( size_t x = 0; x < s.size(); x++ )
  {
    t[x] = s[x] - (factor * Semantics::pow( x, exp ));
  }
  return t;
}

delta_t Reducer::delta( Sequence &seq, int64_t max_delta )
{
  delta_t result;
  result.delta = 0;
  result.offset = 0;
  result.factor = 1;
  const size_t size = seq.size();
  Sequence next;
  next.resize( size );
  for ( int64_t i = 0; i < max_delta; i++ )
  {
    bool ok = true;
    bool same = true;
    for ( size_t j = 0; j < size; j++ )
    {
      number_t p = (j == 0) ? 0 : seq[j - 1];
      if ( p <= seq[j] )
      {
        next[j] = seq[j] - p;
        if ( p != 0 )
        {
          same = false;
        }
      }
      else
      {
        ok = false;
        break;
      }
    }
    if ( ok && !same )
    {
      seq = next;
      result.delta++;
    }
    else
    {
      break;
    }
  }
  result.offset = truncate( seq );
  result.factor = shrink( seq );
//  Log::get().info(
//      "Reduced sequence to " + seq.to_string() + " using delta=" + std::to_string( result.delta ) + ", offset="
//          + std::to_string( result.offset ) + ", factor=" + std::to_string( result.factor ) );
  return result;
}
