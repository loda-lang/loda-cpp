#pragma once

#include "formula.hpp"
#include "sequence.hpp"

class Pari {
 public:
  static std::string generate(const Formula& f);

  static Sequence eval(const Formula& f, int64_t start, int64_t end);
};
