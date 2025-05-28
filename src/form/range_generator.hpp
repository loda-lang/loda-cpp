#pragma once

#include "form/range.hpp"
#include "lang/program.hpp"

/**
 * Range generator. It takes a LODA program as input and generates
 * a range map as output. The range map contains the ranges of memory cells
 * used in the program.
 */
class RangeGenerator {
 public:
  RangeGenerator() = default;

  /**
   * Generates a range map for the given program.
   * @param program The LODA program to analyze.
   * @param ranges The output range map.
   * @return True if the generation was successful, false otherwise.
   */
  bool generate(const Program& program, RangeMap& ranges) const;

 private:
  bool init(const Program& program, RangeMap& ranges) const;
  bool update(const Operation& op, RangeMap& ranges) const;
};
