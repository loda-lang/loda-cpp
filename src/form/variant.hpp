#pragma once

#include <map>
#include <set>
#include <vector>

#include "form/formula.hpp"

/**
 * Variant representation. Variants are used to derive alternative
 * formula representations and thereby to simplify formulas.
 */
class Variant {
 public:
  std::string func;           // function name
  Expression definition;      // function definition
  int64_t num_initial_terms;  // number of required initial terms

  std::set<std::string> used_funcs;      // derived from definition
  std::set<std::string> required_funcs;  // derived from definition
};

/**
 * Variants manager. Keeps a collection of variants and iteratively
 * generates new variants to simplify formulas.
 */
class VariantsManager {
 public:
  VariantsManager(const Formula& formula,
                  const std::map<std::string, int64_t>& num_initial_terms);

  bool update(Variant new_variant);

  std::map<std::string, std::vector<Variant>> variants;

  size_t numVariants() const;

 private:
  void collectFuncs(Variant& variant) const;

  void collectFuncs(Variant& variant, const Expression& expr) const;
};

bool simplifyFormulaUsingVariants(
    Formula& formula, std::map<std::string, int64_t>& num_initial_terms);
