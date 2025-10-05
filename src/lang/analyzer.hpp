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

// Error codes for simple loop extraction
enum class SimpleLoopError {
  OK = 0,
  HAS_INDIRECT_OPERAND = 1,
  MULTIPLE_LOOPS = 2,
  LPB_TARGET_NOT_DIRECT = 3,
  LPB_SOURCE_NOT_ONE = 4,
  LPE_WITHOUT_LPB = 5,
  NO_LOOP_FOUND = 6,
};

class Analyzer {
 public:
  // Check if a program is a simple loop and extract its parts:
  // 1) pre-loop
  // 2) loop body
  // 3) post-loop
  // Optionally returns an error code if the program is not a simple loop
  static SimpleLoopProgram extractSimpleLoop(const Program& program,
                                             SimpleLoopError* error = nullptr);

  // Static code analysis check to find out whether a program consists of a
  // loop that is executed logarithmic time complexity. This is a sufficient
  // but not a necessary check.
  static bool hasLogarithmicComplexity(const Program& program);

  // Static code analysis check to find out whether a program consists of a
  // loop that is executed exponential time complexity. This is a sufficient
  // but not a necessary check.
  static bool hasExponentialComplexity(const Program& program);
};
