#pragma once

#include "eval/evaluator.hpp"
#include "eval/optimizer.hpp"
#include "lang/program.hpp"
#include "sys/util.hpp"

class Minimizer {
 public:
  explicit Minimizer(const Settings &settings)
      : settings(settings),
        optimizer(settings),
        evaluator(settings, EVAL_ALL, false) {}

  bool minimize(Program &p, size_t num_terms) const;

  bool optimizeAndMinimize(Program &p, size_t num_terms) const;

  static int64_t getPowerOf(const Number &v);

 private:
  bool replaceConstantLoop(Program &p, const Sequence &seq, int64_t exp) const;

  bool check(const Program &p, const Sequence &seq, size_t max_total) const;

  Settings settings;
  Optimizer optimizer;
  mutable Evaluator evaluator;
};
