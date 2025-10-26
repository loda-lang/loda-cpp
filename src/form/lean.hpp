#pragma once

#include <set>

#include "form/formula.hpp"
#include "math/sequence.hpp"

class LeanFormula {
 public:
  LeanFormula() {};

  static bool convert(const Formula& formula, int64_t offset, bool as_vector,
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
  Formula main_formula;
  std::string domain;  // Int or Nat
  std::set<std::string> imports;

  static bool initializeLeanProject();

  bool convertToLean(Expression& expr, const Formula& f,
                     const std::vector<std::string>& funcNames);

  std::string printFunction(const std::string& funcName) const;

  void transformParameterReferences(Expression& expr, int64_t offset,
                                    const std::string& funcName) const;
};
