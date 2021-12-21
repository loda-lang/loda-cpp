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

  void mutateOperation(Operation &op, int64_t num_cells);

  void mutateCopies(const Program &program, size_t num_results,
                    std::stack<Program> &result);

  void mutateConstants(const Program &program, size_t num_results,
                       std::stack<Program> &result);

 private:
  ProgramIds program_ids;
  std::vector<Number> constants;
  std::vector<Operation::Type> operation_types;

  std::discrete_distribution<> constants_dist;
  std::discrete_distribution<> operation_types_dist;
};
