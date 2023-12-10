#pragma once

#include "form/formula.hpp"

class FormulaUtil {
 public:
  static void resolveIdentities(Formula& formula);

  static void resolveSimpleFunctions(Formula& formula);

  static void resolveSimpleRecursions(Formula& formula);

  static int64_t getRecursionDepth(const Formula& formula,
                                   const std::string& fname);
};
