#pragma once

#include <vector>

#include "program.hpp"

class Benchmark {
 public:
  void all();

  void operations();

  void programs();

 private:
  void program(size_t id, size_t num_terms);

  std::string programEval(const Program& p, bool use_inc_eval,
                          size_t num_terms);
};
