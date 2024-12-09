#pragma once

#include <map>

#include "eval/evaluator_inc.hpp"
#include "form/formula.hpp"
#include "lang/program.hpp"

/**
 * Formula generator. It takes a LODA program as input and generates
 * a formula as output. This works only for a subclass of programs.
 * The programs must either have no loops or consist only of simple
 * loops, which support incremental evaluation.
 *
 * Example program:
 * mov $1,1
 * lpb $0
 *   mul $1,$0
 *   sub $0,1
 * lpe
 * mov $0,$1
 *
 * Generated formula:
 * a(n) = n*a(n-1), a(0) = 1
 */
class FormulaGenerator {
 public:
  FormulaGenerator();

  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p);

  void initFormula(int64_t numCells, bool useIncEval);

  Expression divToFraction(const Expression& numerator,
                           const Expression& denominator) const;

  bool update(const Operation& op);

  bool update(const Program& p);

  void prepareForPostLoop(int64_t numCells, int64_t offset,
                          const std::map<int64_t, Expression>& preloopExprs);

  bool addInitialTerms(int64_t numCells, int64_t offset,
                       const std::map<std::string, int64_t>& numTerms);

  std::string newName();

  std::string getCellName(int64_t cell) const;

  Expression operandToExpression(Operand op) const;

  void simplifyFunctionNames();

  Settings settings;
  Interpreter interpreter;
  IncrementalEvaluator incEval;

  Formula formula;
  std::map<int64_t, std::string> cellNames;
  size_t freeNameIndex;
  int64_t offset;
};
