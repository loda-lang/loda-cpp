#pragma once

#include <stack>
#include <vector>

#include "form/range.hpp"
#include "lang/program_cache.hpp"

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
  bool generate(const Program& program, RangeMap& ranges);
  bool annotate(Program& program);

 private:
  struct LoopState {
    int64_t counterCell;
    RangeMap rangesBefore;
  };

  bool init(const Program& program, RangeMap& ranges);
  bool update(const Operation& op, RangeMap& ranges);
  bool collect(const Program& program, std::vector<RangeMap>& collected);

  int64_t getTargetCell(const Program& program, size_t index) const;
  int64_t getTargetCell(const Operation& op) const;

  ProgramCache program_cache;
  std::stack<LoopState> loop_states;
};
