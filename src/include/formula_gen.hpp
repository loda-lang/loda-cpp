#pragma once

#include <map>

#include "formula.hpp"
#include "program.hpp"

class FormulaGenerator {
 public:
  FormulaGenerator(bool pariMode);

  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p, Formula& result);

  bool update(Formula& f, const Operation& op) const;

  bool update(Formula& f, const Program& p) const;

  Expression operandToExpression(Operand op) const;

  Formula initFormula(int64_t numCells, bool use_ie) const;

  std::string getCellName(int64_t cell) const;

  const bool pariMode;
  std::map<int64_t, std::string> cellNames;
};
