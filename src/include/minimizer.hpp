#pragma once

#include "evaluator.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class Minimizer {
 public:
  explicit Minimizer(const Settings &settings)
      : settings(settings), optimizer(settings), evaluator(settings) {}

  bool minimize(Program &p, size_t num_terms) const;

  bool optimizeAndMinimize(Program &p, size_t num_terms) const;

  static int64_t getPowerOf(const Number &v);

 private:
  bool replaceClr(Program &p) const;

  bool replaceConstantLoop(Program &p, const Sequence &seq, int64_t exp) const;

  bool check(const Program &p, const Sequence &seq, size_t max_total) const;

  Settings settings;
  Optimizer optimizer;
  mutable Evaluator evaluator;
};
