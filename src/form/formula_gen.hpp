#pragma once

#include <map>

#include "form/formula.hpp"
#include "lang/evaluator_inc.hpp"
#include "lang/program.hpp"

typedef std::multimap<Expression, Expression> Alternatives;

class FormulaGenerator {
 public:
  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p);

  void initFormula(int64_t numCells, bool use_ie,
                   const IncrementalEvaluator& ie);

  bool update(const Operation& op);

  bool update(const Program& p);

  bool resolve(const Alternatives& alt, const Expression& left,
               Expression& right) const;

  bool findAlternatives1(Alternatives& alt) const;

  bool findAlternatives2(Alternatives& alt) const;

  bool applyAlternatives(const Alternatives& alt, Formula& f) const;

  std::string newName();

  std::string getCellName(int64_t cell) const;

  Expression operandToExpression(Operand op) const;

  void simplifyFunctionNames();

  Formula formula;
  std::map<int64_t, std::string> cellNames;
  size_t freeNameIndex;
};
