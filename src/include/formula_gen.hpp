#pragma once

#include <map>

#include "formula.hpp"
#include "program.hpp"

class FormulaGenerator {
 public:
  FormulaGenerator(bool pariMode);

  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p);

  void initFormula(int64_t numCells, bool use_ie);

  bool update(const Operation& op);

  bool update(const Program& p);

  void resolve(const Expression& left, Expression& right) const;

  std::string getCellName(int64_t cell) const;

  Expression operandToExpression(Operand op) const;

  void convertInitialTermsToIf();

  void simplifyFunctionNames(int64_t numCells);

  void restrictToMain();

  void resolveIdentities();

  const bool pariMode;
  Formula formula;
  std::map<int64_t, std::string> cellNames;
};
