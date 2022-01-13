#pragma once

#include "interpreter.hpp"

class IncrementalEvaluator {
 public:
  IncrementalEvaluator(Interpreter& interpreter);

  bool init(const Program& program);

  Number next();

 private:
  void run(const Program& p, Memory& state);

  Interpreter& interpreter;
  Program pre_loop;
  Program loop_body;
  Program post_loop;
  int64_t arg, previous_loop_count;
  Memory init_state;
  Memory loop_state;
};
