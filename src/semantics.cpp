#include "semantics.hpp"

#include "number.hpp"

#define CHECK_INF1(a) if ( a == NUM_INF ) return NUM_INF;
#define CHECK_INF2(a,b) if ( a == NUM_INF || b == NUM_INF ) return NUM_INF;
#define CHECK_ZERO1(a) if ( a == 0 ) return NUM_INF;

number_t Semantics::add( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  if ( (a > 0 && b >= NUM_INF - a) || (a < 0 && -b >= NUM_INF + a) )
  {
    return NUM_INF;
  }
  return a + b;
}

number_t Semantics::sub( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  return add( a, -b );
}

number_t Semantics::trn( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  return std::max<number_t>( add( a, -b ), 0 );
}

number_t Semantics::mul( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  if ( b != 0 && (NUM_INF / std::abs( b ) < std::abs( a )) )
  {
    return NUM_INF;
  }
  return a * b;
}

number_t Semantics::div( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  CHECK_ZERO1( b );
  return a / b;
}

number_t Semantics::dif( number_t a, number_t b )
{
  number_t d = div( a, b );
  if ( d == NUM_INF )
  {
    return NUM_INF;
  }
  return (a == b * d) ? d : a;
}

number_t Semantics::mod( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  CHECK_ZERO1( b );
  return a % b;
}

number_t Semantics::pow( number_t base, number_t exp )
{
  CHECK_INF2( base, exp );
  if ( base == 0 )
  {
    if ( exp > 0 )
    {
      return 0; // 0^(positive number)
    }
    else if ( exp == 0 )
    {
      return 1; // 0^0
    }
    else
    {
      return NUM_INF; // 0^(negative number)
    }
  }
  else if ( base == 1 )
  {
    return 1; // 1^x is always 1
  }
  else if ( base == -1 )
  {
    return (exp % 2 == 0) ? 1 : -1; // (-1)^x
  }
  else
  {
    if ( exp < 0 )
    {
      return 0;
    }
    else
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
}

number_t Semantics::log( number_t n, number_t base )
{
  CHECK_INF2( n, base );
  if ( n <= 0 )
  {
    return NUM_INF;
  }
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

number_t Semantics::fac( number_t a )
{
  CHECK_INF1( a );
  number_t res = 1;
  while ( a != 0 && res != NUM_INF )
  {
    res = mul( res, a );
    a = (a > 0) ? (a - 1) : (a + 1);
  }
  return res;
}

number_t Semantics::gcd( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  if ( a == 0 && b == 0 )
  {
    return NUM_INF;
  }
  a = std::abs( a );
  b = std::abs( b );
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
  CHECK_INF2( n, k );

  // check for negative arguments: https://arxiv.org/pdf/1105.3689.pdf
  number_t sign = 1;
  if ( n < 0 ) // Theorem 2.1
  {
    if ( k >= 0 )
    {
      sign = (k % 2 == 0) ? 1 : -1;
      n = -n + k - 1;
    }
    else if ( k <= n )
    {
      sign = ((n - k) % 2 == 0) ? 1 : -1;
      auto n_old = n;
      n = -k - 1;
      k = n_old - k;
    }
    else
    {
      return 0;
    }
  }
  if ( k < 0 || k > n ) // 1.2
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
  return sign * r;
}

number_t Semantics::cmp( number_t a, number_t b )
{
  CHECK_INF2( a, b );
  return (a == b) ? 1 : 0;
}
