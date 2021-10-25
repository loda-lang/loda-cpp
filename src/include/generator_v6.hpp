#pragma once

#include "generator.hpp"
#include "mutator.hpp"
#include "util.hpp"

class GeneratorV6 : public Generator {
 public:
  GeneratorV6(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

 private:
  AdaptiveScheduler scheduler;
  Mutator mutator;
  Program program;

  void nextProgram();
};
