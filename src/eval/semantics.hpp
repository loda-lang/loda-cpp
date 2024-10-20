#pragma once

#include "math/number.hpp"

class Semantics {
 public:
  static Number add(const Number& a, const Number& b);

  static Number sub(const Number& a, const Number& b);

  static Number trn(const Number& a, const Number& b);

  static Number mul(const Number& a, const Number& b);

  static Number div(const Number& a, const Number& b);

  static Number dif(const Number& a, const Number& b);

  static Number mod(const Number& a, const Number& b);

  static Number pow(const Number& base, const Number& exp);

  static Number gcd(const Number& a, const Number& b);

  static Number lex(const Number& a, const Number& b);

  static Number bin(const Number& n, const Number& k);

  static Number log(const Number& a, const Number& b);

  static Number nrt(const Number& a, const Number& b);

  static Number dis(const Number& a, const Number& b);

  static Number dir(const Number& a, const Number& b);

  static Number equ(const Number& a, const Number& b);

  static Number neq(const Number& a, const Number& b);

  static Number leq(const Number& a, const Number& b);

  static Number geq(const Number& a, const Number& b);

  static Number min(const Number& a, const Number& b);

  static Number max(const Number& a, const Number& b);

  static Number ban(const Number& a, const Number& b);

  static Number bor(const Number& a, const Number& b);

  static Number bxo(const Number& a, const Number& b);

  static Number abs(const Number& a);

  static Number getPowerOf(Number value, const Number& base);
};
