#pragma once

#include "expression.hpp"

class ExpressionUtil {
 public:
  static bool normalize(Expression& e);

  static bool canBeNegative(const Expression& e);
};
