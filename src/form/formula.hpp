#pragma once

#include <map>
#include <vector>

#include "form/expression.hpp"

/**
 * Formula repesentation. A formula consists of a map of expressions. Each
 * map entry defines an equation assigning a left-hand side (LHS) to a
 * right-hand side (RHS) expression. The LHS should be a function declaration
 * and can either use free variables or constants for initial terms of recursive
 * definitions. The RHS is the function definition and may include recursive
 * function calls or references to other functions in this formula.
 *
 * Example: a(n) = b(n)/2, b(n) = b(n-1)+b(n-2), b(1) = 1, b(0) = 1
 */
class Formula {
 public:
  std::string toString(const std::string& sep = ", ",
                       bool brackets = false) const;

  void clear();

  bool contains(const Expression& search) const;

  void replaceAll(const Expression& from, const Expression& to);

  void replaceInside(const Expression& from, const Expression& to,
                     const Expression::Type type);

  void replaceName(const std::string& from, const std::string& to);

  void collectEntries(const std::string& name, Formula& target);

  void collectEntries(const Expression& e, Formula& target);

  std::map<Expression, Expression> collectFunctionEntries(
      const std::string& funcName) const;

  std::map<Expression, Expression> entries;
};
