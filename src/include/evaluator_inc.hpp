#pragma once

#include <set>

#include "interpreter.hpp"

// Incremental Evaluator (IE) class for simple loop programs. It does not work
// as a general evaluator for LODA programs, but only for a certain set of
// programs that contain a simple loop that aggregates a result. IE is much
// faster than regular evaluation, because the result is computed incrementally,
// i.e., for every computing the next term of a sequence, the loop body needs
// to be executed only once. This works by remembering the state of the previous
// iteration and updating it, instead of computing it from scratch.
// The decision whether IA works for a given program is made using a static code
// analysis of the program to be executed.
//
// To find out whether your program is supported by IE, use the init() function.
// If it returns true, use successive calls to the next() function to compute
// the next terms.
//
class IncrementalEvaluator {
 public:
  IncrementalEvaluator(Interpreter& interpreter);

  // Initialize the IE using a program. IE can be applied only if this function
  // returns true.
  bool init(const Program& program);

  // Compute the next term and step count.
  std::pair<Number, size_t> next();

 private:
  void reset();
  bool extractFragments(const Program& program);
  bool checkPreLoop();
  bool checkLoopBody();
  bool checkPostLoop();
  bool updateStatefulCells();
  bool updateLoopCounterDependentCells();

  Interpreter& interpreter;

  // program fragments and metadata
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  std::map<int64_t, Operation::Type> aggregation_types;
  std::set<int64_t> aggregation_cells;
  std::set<int64_t> stateful_cells;
  std::set<int64_t> loop_counter_dependent_cells;
  int64_t loop_counter_cell;
  bool initialized;

  // runtime data
  int64_t argument;
  int64_t previous_loop_count;
  size_t total_loop_steps;
  Memory tmp_state;
  Memory loop_state;
};
