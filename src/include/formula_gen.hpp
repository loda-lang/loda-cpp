#pragma once

#include <map>

#include "formula.hpp"
#include "program.hpp"

typedef std::multimap<Expression, Expression> Alternatives;

class FormulaGenerator {
 public:
  bool generate(const Program& p, int64_t id, Formula& result, bool withDeps);

 private:
  bool generateSingle(const Program& p);

  void initFormula(int64_t numCells, bool use_ie);

  bool update(const Operation& op);

  bool update(const Program& p);

  bool resolve(const Alternatives& alt, const Expression& left,
               Expression& right) const;

  bool findAlternatives(Alternatives& alt) const;

  bool applyAlternatives(const Alternatives& alt, Formula& f) const;

  std::string newName();

  std::string getCellName(int64_t cell) const;

  Expression operandToExpression(Operand op) const;

  void convertInitialTermsToIf();

  bool convertArithToPari();

  void simplifyFunctionNames();

  Formula formula;
  std::map<int64_t, std::string> cellNames;
  size_t freeNameIndex;
};
