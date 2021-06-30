#include "semantics.hpp"

#include "number.hpp"

#include <stdexcept>

#define CHECK_IS_BIG(a,b) if (a.is_big || b.is_big) throw std::runtime_error("Bigint not supported for this operation");
#define CHECK_INF1(a) if ( a == Number::INF ) return Number::INF;
#define CHECK_INF2(a,b) if ( a == Number::INF || b == Number::INF ) return Number::INF;
#define CHECK_ZERO1(a) if ( a == Number::ZERO ) return Number::INF;

// TODO: remove this once we switched to bigint
const int64_t Semantics::NUM_INF = std::numeric_limits<int64_t>::max();

Number Semantics::add( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  if ( (a.value > 0 && b.value >= NUM_INF - a.value) || (a.value < 0 && -b.value >= NUM_INF + a.value) )
  {
    return Number::INF;
  }
  return Number( a.value + b.value );
}

Number Semantics::sub( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  return add( a, Number( -b.value ) );
}

Number Semantics::trn( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  return max( sub( a, b ), Number::ZERO );
}

Number Semantics::mul( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  if ( b.value != 0 && (NUM_INF / std::abs( b.value ) < std::abs( a.value )) )
  {
    return Number::INF;
  }
  return Number( a.value * b.value );
}

Number Semantics::div( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  CHECK_ZERO1( b );
  return Number( a.value / b.value );
}

Number Semantics::dif( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  auto d = div( a, b );
  CHECK_INF1( d );
  return (a.value == b.value * d.value) ? d : a;
}

Number Semantics::mod( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  CHECK_ZERO1( b );
  return Number( a.value % b.value );
}

Number Semantics::pow( const Number& base, const Number& exp )
{
  CHECK_IS_BIG( base, exp );
  CHECK_INF2( base, exp );
  if ( base.value == 0 )
  {
    if ( exp.value > 0 )
    {
      return 0; // 0^(positive number)
    }
    else if ( exp.value == 0 )
    {
      return 1; // 0^0
    }
    else
    {
      return Number::INF; // 0^(negative number)
    }
  }
  else if ( base.value == 1 )
  {
    return 1; // 1^x is always 1
  }
  else if ( base.value == -1 )
  {
    return (exp.value % 2 == 0) ? 1 : -1; // (-1)^x
  }
  else
  {
    if ( exp.value < 0 )
    {
      return 0;
    }
    else
    {
      Number res = 1;
      Number b = base;
      auto e = exp.value;
      while ( res != Number::INF && e > 0 )
      {
        if ( e & 1 )
        {
          res = mul( res, b );
        }
        e >>= 1;
        b = mul( b, b );
      }
      return res;
    }
  }
}

Number Semantics::gcd( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  if ( a == Number::ZERO && b == Number::ZERO )
  {
    return Number::INF;
  }
  auto aa = abs( a );
  auto bb = abs( b );
  Number r;
  while ( bb != Number::ZERO )
  {
    r = mod( aa, bb );
    aa = bb;
    bb = r;
  }
  return aa;
}

Number Semantics::bin( const Number& nn, const Number& kk )
{
  CHECK_IS_BIG( nn, kk );
  CHECK_INF2( nn, kk );
  auto n = nn.value;
  auto k = kk.value;

  // check for negative arguments: https://arxiv.org/pdf/1105.3689.pdf
  Number sign( 1 );
  if ( n < 0 ) // Theorem 2.1
  {
    if ( k >= 0 )
    {
      sign = Number( (k % 2 == 0) ? 1 : -1 );
      n = -n + k - 1;
    }
    else if ( k <= n )
    {
      sign = Number( ((n - k) % 2 == 0) ? 1 : -1 );
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
  Number r( 1 );
  if ( 2 * k > n )
  {
    k = n - k;
  }
  for ( int64_t i = 0; i < k; i++ )
  {
    r = mul( r, Number( n - i ) );
    r = div( r, Number( i + 1 ) );
    if ( r == Number::INF )
    {
      break;
    }
  }
  return mul( sign, r );
}

Number Semantics::cmp( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  return (a == b) ? Number::ONE : Number::ZERO;
}

Number Semantics::min( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  return Number( std::min<int64_t>( a.value, b.value ) );
}

Number Semantics::max( const Number& a, const Number& b )
{
  CHECK_IS_BIG( a, b );
  CHECK_INF2( a, b );
  return Number( std::max<int64_t>( a.value, b.value ) );
}

Number Semantics::abs( const Number& a )
{
  CHECK_IS_BIG( a, a );
  CHECK_INF1( a );
  return Number( std::abs( a.value ) );
}

Number Semantics::getPowerOf( Number value, const Number& base )
{
  CHECK_IS_BIG( value, base );
  int64_t result = 0;
  while ( mod( value, base ) == Number::ZERO )
  {
    result++;
    value = div( value, base );
  }
  return (value == Number::ONE) ? Number( result ) : Number::ZERO;
}
