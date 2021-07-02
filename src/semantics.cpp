#include "semantics.hpp"

#include "number.hpp"

Number Semantics::add( const Number& a, const Number& b )
{
  auto r = a;
  r += b;
  return r;
}

Number Semantics::sub( const Number& a, const Number& b )
{
  return add( a, b.negate() );
}

Number Semantics::trn( const Number& a, const Number& b )
{
  return max( sub( a, b ), Number::ZERO );
}

Number Semantics::mul( const Number& a, const Number& b )
{
  auto r = a;
  r *= b;
  return r;
}

Number Semantics::div( const Number& a, const Number& b )
{
  auto r = a;
  r /= b;
  return r;
}

Number Semantics::dif( const Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF || b == Number::ZERO )
  {
    return Number::INF;
  }
  return (mod( a, b ) == Number::ZERO) ? div( a, b ) : a;
}

Number Semantics::mod( const Number& a, const Number& b )
{
  auto r = a;
  r %= b;
  return r;
}

Number Semantics::pow( const Number& base, const Number& exp )
{
  if ( base == Number::INF || exp == Number::INF )
  {
    return Number::INF;
  }
  if ( base == Number::ZERO )
  {
    if ( Number::ZERO < exp )
    {
      return 0; // 0^(positive number)
    }
    else if ( exp == Number::ZERO )
    {
      return 1; // 0^0
    }
    else
    {
      return Number::INF; // 0^(negative number)
    }
  }
  else if ( base == Number::ONE )
  {
    return 1; // 1^x is always 1
  }
  else if ( base == -1 )
  {
    return (mod( exp, 2 ) == Number::ZERO) ? 1 : -1; // (-1)^x
  }
  else
  {
    if ( exp < Number::ZERO )
    {
      return 0;
    }
    else
    {
      Number res = 1;
      Number b = base;
      Number e = exp;
      while ( res != Number::INF && Number::ZERO < e )
      {
        if ( mod( e, 2 ) == Number::ONE )
        {
          res = mul( res, b );
        }
        e = div( e, 2 );
        b = mul( b, b );
      }
      return res;
    }
  }
}

Number Semantics::gcd( const Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF || (a == Number::ZERO && b == Number::ZERO) )
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
  if ( nn == Number::INF || kk == Number::INF )
  {
    return Number::INF;
  }
  auto n = nn;
  auto k = kk;

  // check for negative arguments: https://arxiv.org/pdf/1105.3689.pdf
  Number sign( 1 );
  if ( n < Number::ZERO ) // Theorem 2.1
  {
    if ( !(k < Number::ZERO) )
    {
      sign = (mod( k, 2 ) == Number::ZERO) ? 1 : -1;
      n = sub( add( n.negate(), k ), Number::ONE );
    }
    else if ( !(n < k) )
    {
      sign = (mod( sub( n, k ), 2 ) == Number::ZERO) ? 1 : -1;
      auto n_old = n;
      n = sub( k.negate(), Number::ONE );
      k = sub( n_old, k );
    }
    else
    {
      return 0;
    }
  }
  if ( k < Number::ZERO || n < k ) // 1.2
  {
    return 0;
  }
  Number r( 1 );
  if ( n < mul( k, 2 ) )
  {
    k = sub( n, k );
  }
  auto l = k.asInt();
  for ( int64_t i = 0; i < l; i++ )
  {
    r = mul( r, sub( n, i ) );
    r = div( r, add( i, 1 ) );
    if ( r == Number::INF )
    {
      break;
    }
  }
  return mul( sign, r );
}

Number Semantics::cmp( const Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    return Number::INF;
  }
  return (a == b) ? 1 : 0;
}

Number Semantics::min( const Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    return Number::INF;
  }
  return (a < b) ? a : b;
}

Number Semantics::max( const Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    return Number::INF;
  }
  return (a < b) ? b : a;
}

Number Semantics::abs( const Number& a )
{
  if ( a == Number::INF )
  {
    return Number::INF;
  }
  return (a < Number::ZERO) ? mul( a, -1 ) : a;
}

Number Semantics::getPowerOf( Number value, const Number& base )
{
  if ( value == Number::INF || base == Number::INF )
  {
    return Number::INF;
  }
  int64_t result = 0;
  while ( mod( value, base ) == Number::ZERO )
  {
    result++;
    value = div( value, base );
  }
  return (value == Number::ONE) ? result : 0;
}
