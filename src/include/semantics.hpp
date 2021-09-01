#pragma once

#include "number.hpp"

class Semantics
{
public:

  static void add( Number& a, const Number& b );

  static void sub( Number& a, const Number& b );

  static void trn( Number& a, const Number& b );

  static void mul( Number& a, const Number& b );

  static void div( Number& a, const Number& b );

  static void dif( Number& a, const Number& b );

  static void mod( Number& a, const Number& b );

  static void pow( Number& base, const Number& exp );

  static void gcd( Number& a, const Number& b );

  static void bin( Number& n, const Number& k );

  static void cmp( Number& a, const Number& b );

  static void min( Number& a, const Number& b );

  static void max( Number& a, const Number& b );

  static void abs( Number& a );

  static Number getPowerOf( Number value, const Number& base );

};
