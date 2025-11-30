#pragma once

#include "form/formula.hpp"

class FormulaUtil {
 public:
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

  // Check if the formula contains mutually recursive functions (cycles of
  // length > 1 in the dependency graph, e.g., A calls B, B calls A)
  static bool hasMutualRecursion(
      const Formula& formula,
      Expression::Type type = Expression::Type::FUNCTION);

  static Number getMinimumBaseCase(const Formula& formula,
                                   const std::string& funcName);

  static void removeFunctionEntries(Formula& formula,
                                    const std::string& funcName);
};
