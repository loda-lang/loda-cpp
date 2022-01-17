#pragma once

#include <set>

#include "interpreter.hpp"

class IncrementalEvaluator {
 public:
  IncrementalEvaluator(Interpreter& interpreter);

  bool init(const Program& program);

  Number next();

 private:
  void reset();
  bool extractFragments(const Program& program);
  bool checkPreLoop();
  bool checkLoopBody();
  bool checkPostLoop();
  void runFragment(const Program& fragment, Memory& state);

  Interpreter& interpreter;

  // program fragments and metadata
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  std::set<int64_t> aggregation_cells;
  int64_t loop_counter_cell;
  bool initialized;

  // runtime data
  int64_t argument;
  int64_t previous_loop_count;
  Memory tmp_state;
  Memory loop_state;
};
