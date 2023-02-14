#pragma once

#include "form/expression.hpp"

class ExpressionUtil {
 public:
  static bool normalize(Expression& e);

  static bool isSimpleFunction(const Expression& e);

  static bool canBeNegative(const Expression& e);
};
