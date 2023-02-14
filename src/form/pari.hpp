#pragma once

#include "form/formula.hpp"
#include "lang/sequence.hpp"

class Pari {
 public:
  static bool convertToPari(Formula& f);

  static std::string toString(const Formula& f);

  static Sequence eval(const Formula& f, int64_t start, int64_t end);
};
