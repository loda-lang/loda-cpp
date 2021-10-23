#pragma once

#include <stack>

#include "number.hpp"
#include "program.hpp"
#include "stats.hpp"

class Mutator {
 public:
  Mutator(const Stats &stats);

  void mutateRandom(Program &program);

  void mutateConstants(const Program &program, size_t num_results,
                       std::stack<Program> &result);
};
