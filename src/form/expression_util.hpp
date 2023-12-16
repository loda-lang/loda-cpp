#pragma once

#include <set>

#include "form/expression.hpp"

class ExpressionUtil {
 public:
  static Expression newConstant(int64_t value);

  static Expression newParameter();

  static Expression newFunction(const std::string& name);

  static bool normalize(Expression& e);

  static bool isSimpleFunction(const Expression& e, bool strict = true);

  static bool canBeNegative(const Expression& e);

  static void collectNames(const Expression& e, Expression::Type type,
                           std::set<std::string>& target);
};
