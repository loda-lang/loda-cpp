#pragma once

#include <stack>

#include "number.hpp"
#include "program.hpp"

class Mutator {
 public:
  void mutateConstants(const Program &program, size_t num_results,
                       std::stack<Program> &result);
};
