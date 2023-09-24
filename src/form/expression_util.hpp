#pragma once

#include "form/expression.hpp"

class ExpressionUtil {
 public:
  static Expression newConstant(int64_t value);

  static Expression newParameter();

  static Expression newFunction(const std::string& name);

  static bool normalize(Expression& e);

  static bool isSimpleFunction(const Expression& e);

  static bool canBeNegative(const Expression& e);
};
