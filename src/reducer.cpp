#include "reducer.hpp"

#include "semantics.hpp"

int Reducer::truncate( Sequence &seq )
{
  int offset = seq.min();
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    seq[i] = seq[i] - offset;
  }
  return offset;
}

int Reducer::shrink( Sequence &seq )
{
  int factor = -1;
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    if ( seq[i] > 0 )
    {
      if ( factor == -1 )
      {
        factor = seq[i];
      }
      else
      {
        factor = Semantics::gcd( factor, seq[i] );
      }
    }
  }
  if ( factor == -1 )
  {
    factor = 1;
  }
  if ( factor > 1 )
  {
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      seq[i] = seq[i] / factor;
    }
  }
  return factor;
}

Sequence subPoly( const Sequence &s, int64_t factor, int64_t exp )
{
  Sequence t;
  t.resize( s.size() );
  for ( size_t x = 0; x < s.size(); x++ )
  {
    t[x] = s[x] - (factor * Semantics::pow( x, exp ));
  }
  return t;
}

Polynomial Reducer::polynomial( Sequence &seq, int64_t degree )
{
  // recursion end
  if ( degree < 0 )
  {
    return Polynomial();
  }

  // calculate maximum factor for current degree
  int64_t max_factor = -1;
  for ( size_t x = 0; x < seq.size(); x++ )
  {
    auto x_exp = Semantics::pow( x, degree );
    int64_t new_factor = (x_exp == 0) ? -1 : seq[x] / x_exp;
    max_factor = (max_factor == -1) ? new_factor : (new_factor < max_factor ? new_factor : max_factor);
    if ( max_factor == 0 ) break;
  }

  int64_t factor = max_factor;
  Sequence reduced = subPoly( seq, factor, degree );
  auto poly = polynomial( reduced, degree - 1 );
  auto cost = reduced.sum();

  int64_t min_factor = std::max( (int64_t) 0, factor - 8 ); // magic number
  while ( factor > min_factor )
  {
    Sequence reduced_new = subPoly( seq, factor - 1, degree );
    auto poly_new = polynomial( reduced_new, degree - 1 );
    auto cost_new = reduced_new.sum();
    if ( cost_new < cost )
    {
      factor--;
      reduced = reduced_new;
      poly = poly_new;
      cost = cost_new;
    }
    else
    {
      break;
    }
  }
  poly.push_back( factor );
  seq = reduced;
  return poly;
}
