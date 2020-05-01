#include "semantics.hpp"

#include "number.hpp"

number_t Semantics::add( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF && NUM_INF - a > b )
  {
    return a + b;
  }
  return NUM_INF;
}

number_t Semantics::sub( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF )
  {
    if ( a > b )
    {
      return a - b;
    }
    else
    {
      return 0;
    }
  }
  return NUM_INF;
}

number_t Semantics::mul( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF && (b == 0 || (NUM_INF / b >= a)) )
  {
    return a * b;
  }
  return NUM_INF;
}

number_t Semantics::div( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF && b != 0 )
  {
    return a / b;
  }
  return NUM_INF;
}

number_t Semantics::mod( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF && b != 0 )
  {
    return a % b;
  }
  return NUM_INF;
}

number_t Semantics::pow( number_t base, number_t exp )
{
  if ( base != NUM_INF && exp != NUM_INF )
  {
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
  }
  return NUM_INF;
}

number_t Semantics::fac( number_t a )
{
  if ( a != NUM_INF )
  {
    number_t res = 1;
    while ( a > 1 && res != NUM_INF )
    {
      res = mul( res, a );
      a--;
    }
    return res;
  }
  return NUM_INF;
}

number_t Semantics::gcd( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF )
  {
    number_t r;
    while ( b != 0 )
    {
      r = a % b;
      a = b;
      b = r;
    }
    return a;
  }
  return NUM_INF;
}

number_t Semantics::cmp( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF )
  {
    return (a == b) ? 1 : 0;
  }
  return NUM_INF;
}
