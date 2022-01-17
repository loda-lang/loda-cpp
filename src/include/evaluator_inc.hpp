#pragma once

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

  void run(const Program& p, Memory& state);

  Interpreter& interpreter;
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  int64_t loop_counter_cell;
  int64_t argument;
  int64_t previous_loop_count;
  bool initialized;
  Memory init_state;
  Memory loop_state;
};
