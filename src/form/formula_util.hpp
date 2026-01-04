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

  static void removeFunctionEntries(Formula& formula,
                                    const std::string& funcName);
};
