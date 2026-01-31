#pragma once

#include <map>
#include <string>
#include <vector>

#include "form/formula.hpp"

class Formula;

/**
 * Represents a function with its general case and base cases.
 *
 * A Function encapsulates:
 * - The function name (e.g., "a", "b")
 * - The general/recursive case: func(n) = expression
 * - Base cases: func(K) = expression for constant values K
 */
class Function {
 public:
  Function(const std::string& name, const Expression& general_case,
           const std::map<int64_t, Expression>& base_cases);

  Function(const std::string& name);

  // Convert a Formula into a list of Functions, extracting all function
  // definitions (general cases and base cases) from the formula.
  // If name is non-empty, only extract the function with this name.
  static std::vector<Function> fromFormula(const Formula& formula,
                                           const std::string& name = "");

  // Get the minimum base case index for this function.
  // Returns -1 if no base cases are defined.
  int64_t getMinimumBaseCase() const;

  std::string name;
  Expression general_case;
  std::map<int64_t, Expression> base_cases;
  bool has_general_case;
};
