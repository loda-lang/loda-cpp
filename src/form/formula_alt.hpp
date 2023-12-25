#pragma once

#include <map>
#include <set>
#include <vector>

#include "form/formula.hpp"

class Variant {
 public:
  std::string func;
  Expression definition;
  std::set<std::string> used_funcs;
  std::set<std::string> required_funcs;
  int64_t num_initial_terms;
};

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
