#pragma once

#include "formula.hpp"
#include "program.hpp"

class FormulaGenerator {
 public:
  FormulaGenerator(bool pariMode);

  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p, Formula& result);

  bool update(Formula& f, const Operation& op);

  bool update(Formula& f, const Program& p);

  const bool pariMode;
};
