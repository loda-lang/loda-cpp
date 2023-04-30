#pragma once

#include "mine/generator.hpp"

class GeneratorV5 : public Generator {
 public:
  GeneratorV5(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

 private:
  Blocks blocks;
  std::discrete_distribution<> dist;
  const size_t length;
};
