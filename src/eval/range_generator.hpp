#pragma once

#include <map>
#include <stack>
#include <vector>

#include "eval/range.hpp"
#include "lang/analyzer.hpp"
#include "lang/program_cache.hpp"

/**
 * RangeGenerator analyzes LODA programs to compute the value ranges for each
 * memory cell. It supports both general programs and simple loop programs, and
 * can annotate programs with computed ranges for debugging or optimization.
 *
 * Usage:
 *   - Set the input upper bound (optional, defaults to infinity)
 *   - Set whether to collect ranges before or after operations (optional, defaults to after)
 *   - Call generate() to get a RangeMap for a program
 *   - Call collect() to get per-operation ranges
 *   - Call annotate() to add range comments to a program
 *   - For simple loops, use the collect(SimpleLoopProgram, ...) variant to get
 * ranges for each phase
 *
 * Example:
 *   RangeGenerator gen;
 *   gen.setInputUpperBound(100);
 *   gen.setRangeBeforeOp(true);  // Optional: get ranges before operations
 *   RangeMap ranges;
 *   gen.generate(program, ranges);
 */
class RangeGenerator {
 public:
  RangeGenerator() : input_upper_bound(Number::INF), is_range_before_op(false) {}

  /**
   * Computes the final range map for all memory cells after running the
   * program.
   * @param program The LODA program to analyze.
   * @param ranges Output: map from cell index to computed Range.
   * @return True if successful, false otherwise.
   */
  bool generate(const Program& program, RangeMap& ranges);

  /**
   * Annotates each operation in the program with a comment describing the range
   * of its target cell.
   * @param program The LODA program to annotate (comments will be added).
   * @return True if successful, false otherwise.
   */
  bool annotate(Program& program);

  /**
   * Computes the range map after each operation in the program (or before each
   * operation if setRangeBeforeOp(true) was called).
   * @param program The LODA program to analyze.
   * @param ranges Output: vector of RangeMap, one per operation.
   * @return True if successful, false otherwise.
   */
  bool collect(const Program& program, std::vector<RangeMap>& ranges);

  /**
   * Computes range maps for each phase of a SimpleLoopProgram (pre-loop, body,
   * post-loop).
   * @param loop The SimpleLoopProgram to analyze.
   * @param pre_loop_ranges Output: range maps for pre-loop operations.
   * @param body_ranges Output: range maps for loop body operations.
   * @param post_loop_ranges Output: range maps for post-loop operations.
   * @return True if successful, false otherwise.
   */
  bool collect(const SimpleLoopProgram& loop,
               std::vector<RangeMap>& pre_loop_ranges,
               std::vector<RangeMap>& body_ranges,
               std::vector<RangeMap>& post_loop_ranges);

  /**
   * Sets the upper bound for the input cell (cell 0) used in range analysis.
   * @param bound The upper bound value (default is infinity).
   */
  void setInputUpperBound(const Number& bound) { input_upper_bound = bound; }

  /**
   * Sets whether ranges should be collected before or after operations.
   * @param before If true, collect() returns ranges before each operation is executed.
   *               If false (default), ranges are collected after each operation.
   */
  void setRangeBeforeOp(bool before) { is_range_before_op = before; }

 private:
  struct LoopState {
    int64_t counterCell;
    RangeMap rangesBefore;
  };

  bool init(const Program& program, RangeMap& ranges);
  bool update(const Operation& op, RangeMap& ranges);

  bool handleSeqOperation(const Operation& op, Range& target);
  void mergeLoopRange(const Range& before, Range& target) const;

  int64_t getTargetCell(const Program& program, size_t index) const;
  int64_t getTargetCell(const Operation& op) const;

  Number input_upper_bound;
  bool is_range_before_op;
  ProgramCache program_cache;
  std::map<UID, Range> seq_range_cache;
  std::stack<LoopState> loop_states;
};
