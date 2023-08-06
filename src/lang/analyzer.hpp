#pragma once

#include "lang/program.hpp"

class SimpleLoopProgram {
 public:
  bool is_simple_loop;
  Program pre_loop;
  Program body;
  Program post_loop;
  int64_t counter;
};

class Analyzer {
 public:
  // Check if a program is a simple loop and extract its parts.
  static SimpleLoopProgram extractSimpleLoop(const Program& program);

  // Static code analysis check to find out whether a program consists of a
  // loop that is executed logarithmic time complexity. This is a sufficient
  // but not a necessary check.
  static bool hasLogarithmicComplexity(const Program& program);

  // Static code analysis check to find out whether a program consists of a
  // loop that is executed exponential time complexity. This is a sufficient
  // but not a necessary check.
  static bool hasExponentialComplexity(const Program& program);
};
