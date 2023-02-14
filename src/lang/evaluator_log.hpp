#pragma once

#include "lang/program.hpp"

// Logarithmic Evaluator is not a real evaluator, but a static code analysis
// utility to find out whether a program consists of a loop that is executed
// in O(log(n)) time complexity. This is done by inspecting the operations on
// the loop counter cell and ensuring that it gets divided by a constant >1
// in every iteration.
//
class LogarithmicEvaluator {
 public:
  static bool hasLogarithmicComplexity(const Program& program);
};
