#pragma once

#include <map>
#include <set>

#include "interpreter.hpp"

class IncrementalEvaluator {
 public:
  IncrementalEvaluator(Interpreter& interpreter);

  bool init(const Program& program);

  std::pair<Number, size_t> next();

 private:
  void reset();
  bool extractFragments(const Program& program);
  bool checkPreLoop();
  bool checkLoopBody();
  bool checkPostLoop();
  bool addDependency(int64_t from, int64_t to);
  bool hasDependency(int64_t from, int64_t to) const;

  size_t runFragment(const Program& fragment, Memory& state);

  Interpreter& interpreter;

  // program fragments and metadata
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  std::set<int64_t> aggregation_cells;
  std::multimap<int64_t, int64_t> depends_on;
  int64_t loop_counter_cell;
  bool initialized;

  // runtime data
  int64_t argument;
  int64_t previous_loop_count;
  size_t total_loop_steps;
  Memory tmp_state;
  Memory loop_state;
};
