#pragma once

#include "form/formula.hpp"
#include "lang/sequence.hpp"

/**
 * PARI/GP generator. Takes a formula as input and generates
 * executable PARI/GP code as output. It also includes a utility
 * function to invoke the GP interpreter.
 *
 * Example input formula:
 * a(n) = n*a(n-1), a(0) = 1
 *
 * Generated PARI/GP code:
 * a(n) = if(n==0,1,n*a(n-1))
 */
class Pari {
 public:
  static bool convertToPari(Formula& f, bool as_vector = false);

  static Sequence eval(const Formula& f, int64_t start, int64_t end);
};
