#pragma once

#include "form/formula.hpp"
#include "math/sequence.hpp"

class LeanFormula {
 public:
  LeanFormula() {};

  static bool convert(const Formula& formula, bool as_vector,
                      LeanFormula& lean_formula);

  std::string toString() const;

  std::string printEvalCode(int64_t offset, int64_t numTerms) const;

  // Evaluates the formula for the given offset and number of terms, with a
  // timeout in seconds. Returns true if successful, false if a timeout
  // occurred. The result sequence is written to 'result'.
  bool eval(int64_t offset, int64_t numTerms, int timeoutSeconds,
            Sequence& result) const;

  std::string getName() const { return "LEAN"; }

 private:
  bool needsBitwiseImport() const;

  std::string getImports() const;

  static bool initializeLeanProject();

  Formula main_formula;
};
