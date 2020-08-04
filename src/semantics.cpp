#include "semantics.hpp"

#include "number.hpp"

#define CHECK_INF(a,b) if ( a == NUM_INF || b == NUM_INF ) return NUM_INF;

number_t Semantics::add( number_t a, number_t b )
{
  CHECK_INF( a, b );
  if ( NUM_INF - a > b )
  {
    return a + b;
  }
  return NUM_INF;
}

number_t Semantics::sub( number_t a, number_t b )
{
  CHECK_INF( a, b );
  if ( a > b )
  {
    return a - b;
  }
  else
  {
    return 0;
  }
}

number_t Semantics::mul( number_t a, number_t b )
{
  CHECK_INF( a, b );
  if ( b == 0 || (NUM_INF / b >= a) )
  {
    return a * b;
  }
  return NUM_INF;
}

number_t Semantics::div( number_t a, number_t b )
{
  CHECK_INF( a, b );
  if ( b != 0 )
  {
    return a / b;
  }
  return NUM_INF;
}

number_t Semantics::mod( number_t a, number_t b )
{
  CHECK_INF( a, b );
  if ( b != 0 )
  {
    return a % b;
  }
  return NUM_INF;
}

number_t Semantics::pow( number_t base, number_t exp )
{
  CHECK_INF( base, exp );
  if ( base == 0 )
  {
    return (exp == 0) ? 1 : 0;
  }
  else if ( base == 1 )
  {
    return 1;
  }
  else if ( base > 1 )
  {
    number_t res = 1;
    while ( res != NUM_INF && exp > 0 )
    {
      if ( exp & 1 )
      {
        res = mul( res, base );
      }
      exp >>= 1;
      base = mul( base, base );
    }
    return res;
  }
  return NUM_INF;
}

number_t Semantics::log( number_t n, number_t base )
{
  CHECK_INF( n, base );
  if ( n != 0 )
  {
    if ( n == 1 )
    {
      return 0;
    }
    if ( base < 2 )
    {
      return NUM_INF;
    }
    number_t m = 1;
    number_t res = 0;
    while ( m < n )
    {
      m = mul( m, base );
      res++;
    }
    return (m == n) ? res : res - 1;
  }
  return NUM_INF;
}

number_t Semantics::fac( number_t a )
{
  CHECK_INF( a, a );
  number_t res = 1;
  while ( a > 1 && res != NUM_INF )
  {
    res = mul( res, a );
    a--;
  }
  return res;
}

number_t Semantics::gcd( number_t a, number_t b )
{
  CHECK_INF( a, b );
  number_t r;
  while ( b != 0 )
  {
    r = a % b;
    a = b;
    b = r;
  }
  return a;
}

number_t Semantics::bin( number_t n, number_t k )
{
  CHECK_INF( n, k );
  if ( k > n )
  {
    return 0;
  }
  number_t r = 1;
  if ( 2 * k > n )
  {
    k = n - k;
  }
  for ( number_t i = 0; i < k; i++ )
  {
    r = mul( r, n - i );
    r = div( r, i + 1 );
    if ( r == NUM_INF )
    {
      break;
    }
  }
  return r;
}

number_t Semantics::cmp( number_t a, number_t b )
{
  CHECK_INF( a, b );
  return (a == b) ? 1 : 0;
}
