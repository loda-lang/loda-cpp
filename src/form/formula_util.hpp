#pragma once

#include "form/formula.hpp"

class FormulaUtil {
 public:
  static void resolveIdentities(Formula& formula);

  static void resolveSimpleFunctions(Formula& formula);

  static void resolveSimpleRecursions(Formula& formula);

  static void convertInitialTermsToIf(Formula& formula);

  static Formula extractInitialTerms(Formula& formula);
};
