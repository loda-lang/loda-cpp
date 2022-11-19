#pragma once

#include <vector>

#include "program.hpp"

class Benchmark {
 public:
  void smokeTest();

  void operations();

  void programs();

  void findSlow();

 private:
  void program(size_t id, size_t num_terms);

  std::string programEval(const Program& p, bool use_inc_eval,
                          size_t num_terms);
};
