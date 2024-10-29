#pragma once

#include <map>
#include <set>

#include "form/expression.hpp"

/**
 * Expression utility functions.
 */
class ExpressionUtil {
 public:
  static Expression newConstant(int64_t value);

  static Expression newParameter();

  static Expression newFunction(const std::string& name);

  static bool normalize(Expression& e);

  static bool isSimpleFunction(const Expression& e, bool strict = true);

  static bool isInitialTerm(const Expression& e);

  static bool isRecursionArgument(const Expression& e, int64_t max_offset);

  static bool isNonRecursiveFunctionReference(
      const Expression& e, const std::vector<std::string>& names,
      int64_t max_offset);

  static bool hasNonRecursiveFunctionReference(
      const Expression& e, const std::vector<std::string>& names,
      int64_t max_offset);

  static bool canBeNegative(const Expression& e);

  static void collectNames(const Expression& e, Expression::Type type,
                           std::set<std::string>& target);

  static Number eval(const Expression& e,
                     const std::map<std::string, Number> params);
};
