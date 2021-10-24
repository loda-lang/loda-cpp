#pragma once

#include <stack>

#include "distribution.hpp"
#include "number.hpp"
#include "program.hpp"
#include "stats.hpp"

class Mutator {
 public:
  Mutator(const Stats &stats);

  void mutateRandom(Program &program);

  void mutateConstants(const Program &program, size_t num_results,
                       std::stack<Program> &result);

 private:
  std::vector<bool> found_programs;
  std::vector<Number> constants;
  std::vector<Operation::Type> operation_types;

  std::discrete_distribution<> constants_dist;
  std::discrete_distribution<> operation_types_dist;

  int64_t getRandomProgramId();
};
