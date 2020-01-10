#pragma once

#include "number.hpp"

class Semantics
{
public:

  static number_t mul( number_t a, number_t b );

  static number_t pow( number_t base, number_t exp );

  static number_t fac( number_t n );

  static number_t gcd( number_t a, number_t b );

};
