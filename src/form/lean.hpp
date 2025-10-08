#pragma once

#include "form/formula.hpp"
#include "math/sequence.hpp"

/**
 * LEAN generator. Takes a formula as input and generates
 * executable LEAN code as output. It also includes a utility
 * function to invoke the LEAN interpreter.
 *
 * Example input formula:
 * a(n) = n*a(n-1), a(0) = 1
 *
 * Generated LEAN code:
 * def a : ℕ → ℕ
 *   | 0 => 1
 *   | n + 1 => n * a n
 */
class LeanFormula {
 public:
  LeanFormula() {};

  static bool convert(const Formula& formula, LeanFormula& lean_formula);

  std::string toString() const;

  std::string printEvalCode(int64_t offset, int64_t numTerms) const;

  // Evaluates the formula for the given offset and number of terms, with a
  // timeout in seconds. Returns true if successful, false if a timeout
  // occurred. The result sequence is written to 'result'.
  bool eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
            Sequence& result) const;

  std::string getName() const { return "LEAN"; }

 private:
  Formula main_formula;
};
