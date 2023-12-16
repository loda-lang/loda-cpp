#pragma once

#include <map>
#include <set>
#include <vector>

#include "form/formula.hpp"

class Variant {
 public:
  Expression definition;
  std::set<std::string> used_funcs;
};

class VariantsManager {
 public:
  VariantsManager(const Formula& formula);

  bool update(const std::string& func, const Expression& expr);

  std::map<std::string, std::vector<Variant>> variants;

  size_t numVariants() const;

 private:
  void collectUsedFuncs(const Expression& expr,
                        std::set<std::string>& used_funcs) const;
};

bool simplifyFormulaUsingVariants(Formula& formula);
