#pragma once

#include <vector>

#include "eval/evaluator.hpp"

class Benchmark {
 public:
  void smokeTest();

  void operations();

  void programs();

  void findSlow(int64_t num_terms, Operation::Type type);

 private:
  void program(size_t id, size_t num_terms);

  std::string programEval(const Program& p, eval_mode_t eval_mode,
                          size_t num_terms);
};
