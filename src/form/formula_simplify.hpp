#pragma once

#include <map>
#include <set>
#include <string>

#include "form/expression.hpp"
#include "form/formula.hpp"
#include "math/number.hpp"

class FormulaSimplify {
 public:
  static void resolveIdentities(Formula& formula);

  static void resolveSimpleFunctions(Formula& formula);

  static void resolveSimpleRecursions(Formula& formula);

  static void replaceSimpleRecursiveReferences(Formula& formula);

 private:
  static bool extractArgumentOffset(const Expression& arg, Number& offset);

  static bool containsParameterOutsideFunction(const Expression& expr,
                                               const std::string& funcName);
  static bool isSimpleRecursiveReference(
      const Formula& formula, const std::string& funcName,
      const Expression& rhs, const std::set<std::string>& processedFuncs,
      std::string& refFuncName, Number& offset);

  static void adjustIndexByOffset(Expression& expr, const Number& offset);

  static void performReplacement(
      Formula& formula, const std::string& funcName,
      const std::string& refFuncName, const Number& offset,
      const std::map<Expression, Expression>& refFuncEntries);
};