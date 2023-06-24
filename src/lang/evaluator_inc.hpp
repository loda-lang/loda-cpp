#pragma once

#include <set>

#include "lang/interpreter.hpp"

// Incremental Evaluator (IE) class for simple loop programs. It does not work
// as a general evaluator for LODA programs, but only for a certain set of
// programs that contain simple loops. IE is much faster than regular
// evaluation, because the result is computed incrementally, i.e., for every
// computing the next term of a sequence, the loop body needs to be executed
// only once. This works by remembering the state of the previous iteration and
// updating it, instead of computing it from scratch. The decision whether IA
// works for a given program is made using a static code analysis of the program
// to be executed.
//
// To find out whether your program is supported by IE, use the init() function.
// If it returns true, use successive calls to the next() function to compute
// the next terms.
//
class IncrementalEvaluator {
 public:
  explicit IncrementalEvaluator(Interpreter& interpreter);

  void reset();

  // Initialize the IE using a program. IE can be applied only if this function
  // returns true.
  bool init(const Program& program);

  // Compute the next term and step count.
  std::pair<Number, size_t> next();

  inline const Program& getPreLoop() const { return pre_loop; }
  inline const Program& getLoopBody() const { return loop_body; }
  inline const Program& getPostLoop() const { return post_loop; }
  inline int64_t getLoopCounterCell() const { return loop_counter_cell; }
  inline int64_t getLoopCounterDecrement() const {
    return loop_counter_decrement;
  }
  inline const std::set<int64_t>& getLoopCounterDependentCells() const {
    return loop_counter_dependent_cells;
  }
  inline const std::set<int64_t>& getStatefulCells() const {
    return stateful_cells;
  }
  inline const std::set<int64_t>& getOutputCells() const {
    return output_cells;
  }
  inline const std::vector<Memory>& getLoopStates() const {
    return loop_states;
  }
  inline int64_t getPreviousSlice() const { return previous_slice; }

 private:
  bool extractFragments(const Program& program);
  bool checkPreLoop();
  bool checkLoopBody();
  bool checkPostLoop();
  bool isCommutative(int64_t cell) const;
  bool isCommutative(const std::set<int64_t>& cells) const;
  void computeStatefulCells();
  void computeLoopCounterDependentCells();
  void initRuntimeData();

  Interpreter& interpreter;

  // program fragments and metadata
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  std::set<int64_t> output_cells;
  std::set<int64_t> stateful_cells;
  std::set<int64_t> loop_counter_dependent_cells;
  int64_t loop_counter_cell;
  int64_t loop_counter_decrement;
  Operation::Type loop_counter_type;
  bool initialized;

  // runtime data
  int64_t argument;
  Memory tmp_state;
  std::vector<Memory> loop_states;
  std::vector<int64_t> previous_loop_counts;
  std::vector<size_t> total_loop_steps;
  int64_t previous_slice;
};
