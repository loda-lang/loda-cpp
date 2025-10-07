#pragma once

#include "form/formula.hpp"
#include "math/sequence.hpp"

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
  PariFormula() : as_vector(false) {};

  static bool convert(const Formula& formula, bool as_vector,
                      PariFormula& pari_formula);

  void printEvalCode(int64_t offset, int64_t numTerms, std::ostream& out) const;

  std::string toString() const;

  // Evaluates the formula for the given offset and number of terms, with a
  // timeout in seconds. Returns true if successful, false if a timeout
  // occurred. The result sequence is written to 'result'.
  bool eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
            Sequence& result) const;

  std::string getName() const { return "PARI"; }

 private:
  Formula main_formula;
  bool as_vector;
};
