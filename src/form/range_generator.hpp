#pragma once

#include "form/range.hpp"
#include "lang/program.hpp"

/**
 * Range generator. It takes a LODA program as input and generates
 * a range set as output. The range set contains the ranges of memory cells
 * used in the program.
 */
class RangeGenerator {
 public:
  RangeGenerator() = default;

  /**
   * Generates a range set for the given program.
   * @param program The LODA program to analyze.
   * @param ranges The output range set.
   * @return True if the generation was successful, false otherwise.
   */
  bool generate(const Program& program, RangeSet& ranges) const;

 private:
  bool init(const Program& program, RangeSet& ranges) const;
  bool update(const Operation& op, RangeSet& ranges) const;
};
