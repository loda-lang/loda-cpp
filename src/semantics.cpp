#include "semantics.hpp"

#include "number.hpp"

#include <iostream>

void Semantics::add( Number& a, const Number& b )
{
  a += b;
}

void Semantics::sub( Number& a, const Number& b )
{
  a.negate();
  a += b;
  a.negate();
}

void Semantics::trn( Number& a, const Number& b )
{
  sub( a, b );
  max( a, Number::ZERO );
}

void Semantics::mul( Number& a, const Number& b )
{
  a *= b;
}

void Semantics::div( Number& a, const Number& b )
{
  a /= b;
}

void Semantics::dif( Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF || b == Number::ZERO )
  {
    a = Number::INF;
  }
  else
  {
    auto e = a;
    div( e, b );
    auto f = b;
    mul( f, e );
    if ( a == f )
    {
      a = e;
    }
  }
}

void Semantics::mod( Number& a, const Number& b )
{
  a %= b;
}

void Semantics::pow( Number& base, const Number& exp )
{
  // base is also used to store the result!
  if ( base == Number::INF || exp == Number::INF )
  {
    base = Number::INF;
  }
  else if ( base == Number::ZERO )
  {
    if ( Number::ZERO < exp )
    {
      base = 0; // 0^(positive number)
    }
    else if ( exp == Number::ZERO )
    {
      base = 1; // 0^0
    }
    else
    {
      base = Number::INF; // 0^(negative number)
    }
  }
  else if ( base == -1 )
  {
    base = exp.odd() ? -1 : 1; // (-1)^x
  }
  else if ( base != Number::ONE ) // 1^x is always 1
  {
    if ( exp < Number::ZERO )
    {
      base = 0;
    }
    else
    {
      Number b = base;
      Number e = exp;
      base = 1;
      while ( base != Number::INF && Number::ZERO < e )
      {
        if ( e.odd() )
        {
          mul( base, b );
        }
        div( e, 2 );
        mul( b, b );
        if ( b == Number::INF )
        {
          base = Number::INF;
        }
      }
    }
  }
}

void Semantics::gcd( Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF || (a == Number::ZERO && b == Number::ZERO) )
  {
    a = Number::INF;
  }
  else
  {
    auto c = b;
    abs( a );
    abs( c );
    Number r;
    while ( c != Number::ZERO )
    {
      r = a;
      mod( r, c );
      if ( r == Number::INF )
      {
        a = Number::INF;
        return;
      }
      a = c;
      c = r;
    }
  }
}

void Semantics::bin( Number& nn, const Number& kk )
{
  if ( nn == Number::INF || kk == Number::INF )
  {
    nn = Number::INF;
    return;
  }
  auto n = nn;
  auto k = kk;

  // check for negative arguments: https://arxiv.org/pdf/1105.3689.pdf
  Number sign( 1 );
  Number m;
  if ( n < Number::ZERO ) // Theorem 2.1
  {
    if ( !(k < Number::ZERO) )
    {
      sign = k.odd() ? -1 : 1;
      n += Number::ONE;
      m = k;
      Semantics::sub( m, n );
      n = m;
    }
    else if ( !(n < k) )
    {
      m = n;
      Semantics::sub( m, k );
      sign = m.odd() ? -1 : 1;
      m = n;
      n = k;
      n += Number::ONE;
      n.negate();
      k = m;
      // k = sub( n_old, k );
    }
    else
    {
      nn = 0;
      return;
    }
  }
  if ( k < Number::ZERO || n < k ) // 1.2
  {
    nn = 0;
    return;
  }
  Number r( 1 );
  auto x = k;
  mul( x, 2 );
  if ( n < x )
  {
    x = n;
    sub( x, k );
    k = x;
  }
  if ( k.getNumUsedWords() > 1 )
  {
    nn = Number::INF;
    return;
  }
  auto l = k.asInt();
  for ( int64_t i = 0; i < l; i++ )
  {
    x = n;
    sub( x, i );
    r *= x;
    x = i;
    x += Number::ONE;
    r /= x;
    if ( r == Number::INF )
    {
      break;
    }
  }
  mul( r, sign );
  nn = r;
}

void Semantics::cmp( Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    a = Number::INF;
  }
  else if ( a == b )
  {
    a = 1;
  }
  else
  {
    a = 0;
  }
}

void Semantics::min( Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    a = Number::INF;
  }
  else if ( b < a )
  {
    a = b;
  }
}

void Semantics::max( Number& a, const Number& b )
{
  if ( a == Number::INF || b == Number::INF )
  {
    a = Number::INF;
  }
  else if ( a < b )
  {
    a = b;
  }
}

void Semantics::abs( Number& a )
{
  if ( a == Number::INF )
  {
    a = Number::INF;
  }
  else if ( a < Number::ZERO )
  {
    a.negate();
  }
}

Number Semantics::getPowerOf( Number value, const Number& base )
{
  if ( value == Number::INF || base == Number::INF || base == Number::ZERO )
  {
    return Number::INF;
  }
  Number x;
  int64_t result = 0;
  while ( true )
  {
    x = value;
    mod( x, base );
    if ( x == Number::ZERO )
    {
      break;
    }
    result++;
    div( value, base );
  }
  return (value == Number::ONE) ? result : 0;
}
