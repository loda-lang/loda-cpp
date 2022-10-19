#pragma once

#include "formula.hpp"
#include "sequence.hpp"

class Pari {
 public:
  static Sequence eval(const Formula& f, int64_t start, int64_t end);
};
