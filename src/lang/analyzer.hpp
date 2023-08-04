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

  // Static code analysis utility to find out whether a program consists of a
  // loop that is executed in O(log(n)) time complexity. This is done by
  // inspecting the operations on the loop counter cell and ensuring that it
  // gets divided by a constant >1 in every iteration.
  static bool hasLogarithmicComplexity(const Program& program);
};
