#include "reducer.hpp"

#include "semantics.hpp"
#include "util.hpp"

number_t Reducer::truncate( Sequence &seq )
{
  if ( seq.empty() )
  {
    return 0;
  }
  // get minimum positive value; no negative values are allowed
  auto min = NUM_INF;
  for ( auto v : seq )
  {
    if ( v < 0 )
    {
      return 0;
    }
    else if ( min == NUM_INF || v < min )
    {
      min = v;
    }
  }
  if ( min > 0 && min != NUM_INF )
  {
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      seq[i] = seq[i] - min;
    }
  }
  return min;
}

number_t Reducer::shrink( Sequence &seq )
{
  Number factor = Number::INF;
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    if ( seq[i] != 0 )
    {
      if ( factor == Number::INF )
      {
        factor = Semantics::abs( Number( seq[i] ) );
      }
      else
      {
        factor = Semantics::gcd( factor, Semantics::abs( Number( seq[i] ) ) );
      }
    }
  }
  if ( factor == Number::INF || factor == Number::ZERO )
  {
    factor = Number::ONE;
  }
  if ( factor != Number::ONE )
  {
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      seq[i] = Semantics::div( Number( seq[i] ), factor ).asInt();
    }
  }
  return factor.asInt();
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

int64_t Reducer::digit( Sequence &seq, int64_t num_digits )
{
  Sequence count;
  count.resize( num_digits, 0 );
  for ( auto n : seq )
  {
    count[((n % num_digits) + num_digits) % num_digits]++;
  }
  int64_t index = 0;
  number_t max = 0;
  for ( int64_t i = 0; i < num_digits; i++ )
  {
    if ( count[i] > max )
    {
      index = i;
      max = count[i];
    }
  }
  for ( int64_t i = 0; i < static_cast<int64_t>( seq.size() ); i++ )
  {
    seq[i] = (((seq[i] - index) % num_digits) + num_digits) % num_digits;
  }
//  Log::get().info(
//      "Reduced sequence to " + seq.to_string() + " using num_digits=" + std::to_string( num_digits ) + ", offset="
//          + std::to_string( index ) );
  return index;
}
