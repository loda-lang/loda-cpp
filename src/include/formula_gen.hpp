#pragma once

#include "formula.hpp"
#include "program.hpp"

class FormulaGenerator {
 public:
  static std::pair<bool, Formula> generate(const Program& p, bool pariMode);
};
