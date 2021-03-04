#pragma once

#include "number.hpp"

class Semantics
{
public:

  static number_t add( number_t a, number_t b );

  static number_t sub( number_t a, number_t b );

  static number_t trn( number_t a, number_t b );

  static number_t mul( number_t a, number_t b );

  static number_t div( number_t a, number_t b );

  static number_t dif( number_t a, number_t b );

  static number_t mod( number_t a, number_t b );

  static number_t pow( number_t base, number_t exp );

  static number_t log( number_t n, number_t base );

  static number_t fac( number_t n );

  static number_t gcd( number_t a, number_t b );

  static number_t bin( number_t n, number_t k );

  static number_t cmp( number_t a, number_t b );

};
