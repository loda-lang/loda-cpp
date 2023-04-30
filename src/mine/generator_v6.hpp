#pragma once

#include "mine/generator.hpp"
#include "mine/mutator.hpp"
#include "sys/util.hpp"

class GeneratorV6 : public Generator {
 public:
  GeneratorV6(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

 private:
  AdaptiveScheduler scheduler;
  Mutator mutator;
  Program program;

  void nextProgram();
};
