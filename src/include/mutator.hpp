#pragma once

#include <stack>

#include "distribution.hpp"
#include "number.hpp"
#include "program.hpp"
#include "stats.hpp"

class Mutator {
 public:
  Mutator(const Stats &stats,
          double mutation_rate = 0.3,  // magic number
          bool mutate_comment = false);

  void mutateRandom(Program &program);

  void mutateOperation(Operation &op, int64_t num_cells);

  void mutateCopies(const Program &program, size_t num_results,
                    std::stack<Program> &result);

  void mutateConstants(const Program &program, size_t num_results,
                       std::stack<Program> &result);

  RandomProgramIds2 random_program_ids;

 private:
  const double mutation_rate;
  const bool mutate_comment;
  std::vector<Number> constants;
  std::vector<Operation::Type> operation_types;

  std::discrete_distribution<> constants_dist;
  std::discrete_distribution<> operation_types_dist;

  std::vector<size_t> tmp_comment_positions;
};
