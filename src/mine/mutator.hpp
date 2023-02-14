#pragma once

#include <stack>

#include "mine/distribution.hpp"
#include "lang/number.hpp"
#include "lang/program.hpp"
#include "mine/stats.hpp"

class Mutator {
 public:
  explicit Mutator(const Stats &stats,
                   double mutation_rate = 0.3,  // magic number
                   bool mutate_comment = false);

  void mutateRandom(Program &program);

  void mutateOperation(Operation &op, int64_t num_cells, int64_t new_cells);

  void mutateCopiesRandom(const Program &program, size_t num_results,
                          std::stack<Program> &result);

  void mutateCopiesConstants(const Program &program, size_t num_results,
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
