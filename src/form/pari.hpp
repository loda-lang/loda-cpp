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
class PariFormula {
 public:
  PariFormula() : as_vector(false){};

  static bool convert(const Formula& formula, bool as_vector,
                      PariFormula& pari_formula);

  void printEvalCode(int64_t numTerms, std::ostream& out) const;

  std::string toString() const;

  Sequence eval(int64_t numTerms) const;

 private:
  Formula main_formula;
  bool as_vector;
};
