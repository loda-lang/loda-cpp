#pragma once

#include "form/formula.hpp"

class FormulaUtil {
 public:
  static void resolveIdentities(Formula& formula);

  static void resolveSimpleFunctions(Formula& formula);

  static void resolveSimpleRecursions(Formula& formula);

  static std::vector<std::string> getDefinitions(
      const Formula& formula,
      const Expression::Type type = Expression::Type::FUNCTION,
      bool sortByDependencies = false);

  static std::multimap<std::string, std::string> getDependencies(
      const Formula& formula,
      const Expression::Type type = Expression::Type::FUNCTION,
      bool transitive = false, bool ignoreSelf = false);

  static bool isRecursive(const Formula& formula, const std::string& funcName,
                          Expression::Type type = Expression::Type::FUNCTION);

  static void convertInitialTermsToIf(
      Formula& formula,
      const Expression::Type type = Expression::Type::FUNCTION);

  static void replaceSimpleRecursiveReferences(Formula& formula);
};
