#pragma once

#include <vector>

#include "number.hpp"

class Benchmark {
 public:
  void all();

  void operations();

  void programs();

 private:
  void program(size_t id, size_t num_terms);
};
