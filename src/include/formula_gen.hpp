#pragma once

#include "formula.hpp"
#include "program.hpp"

class FormulaGenerator {
 public:
  static bool generate(const Program& p, int64_t id, Formula& result,
                       bool pariMode, bool withDeps);
};
