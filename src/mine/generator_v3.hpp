#pragma once

#include "mine/generator.hpp"

struct OpProb {
  Operation operation;
  size_t partial_sum;
};

class GeneratorV3 : public Generator {
 public:
  GeneratorV3(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

  virtual bool isFinished() const override;

 private:
  std::discrete_distribution<> length_dist;
  std::vector<std::vector<OpProb>> operation_dists;
};
