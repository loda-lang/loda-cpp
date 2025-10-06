#pragma once

#include <map>
#include <stack>
#include <vector>

#include "eval/range.hpp"
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
  bool generate(const Program& program, RangeMap& ranges,
                Number inputUpperBound = Number::INF);
  
  /**
   * Collects range maps for each operation in the program.
   * @param program The LODA program to analyze.
   * @param collected The output vector of range maps (one per operation).
   * @param inputUpperBound The upper bound for the input parameter.
   * @return True if the collection was successful, false otherwise.
   */
  bool collectRanges(const Program& program, std::vector<RangeMap>& collected,
                     Number inputUpperBound = Number::INF);
  
  bool annotate(Program& program, Number inputUpperBound = Number::INF);

 private:
  struct LoopState {
    int64_t counterCell;
    RangeMap rangesBefore;
  };

  bool init(const Program& program, RangeMap& ranges, Number inputUpperBound);
  bool update(const Operation& op, RangeMap& ranges);
  bool collect(const Program& program, std::vector<RangeMap>& collected,
               Number inputUpperBound);

  int64_t getTargetCell(const Program& program, size_t index) const;
  int64_t getTargetCell(const Operation& op) const;
  void mergeLoopRange(const Range& before, Range& target) const;

  ProgramCache program_cache;
  std::map<UID, Range> seq_range_cache;
  std::stack<LoopState> loop_states;
};
