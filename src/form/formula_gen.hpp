#pragma once

#include <map>

#include "form/formula.hpp"
#include "lang/evaluator_inc.hpp"
#include "lang/program.hpp"

class FormulaGenerator {
 public:
  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p);

  void initFormula(int64_t numCells, bool use_ie,
                   const IncrementalEvaluator& ie);

  bool update(const Operation& op);

  bool update(const Program& p);

  std::string newName();

  std::string getCellName(int64_t cell) const;

  Expression operandToExpression(Operand op) const;

  void simplifyFunctionNames();

  Formula formula;
  std::map<int64_t, std::string> cellNames;
  size_t freeNameIndex;
};
