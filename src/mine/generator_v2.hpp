#pragma once

#include "mine/generator.hpp"

class GeneratorV2 : public Generator {
 public:
  GeneratorV2(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

 private:
  std::discrete_distribution<> length_dist;
  std::discrete_distribution<> operation_dist;
  std::vector<Operation> operations;
};
