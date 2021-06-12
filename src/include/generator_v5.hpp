#pragma once

#include "generator.hpp"

class GeneratorV5: public Generator
{
public:

  GeneratorV5( const Config &config, const Stats &stats, int64_t seed );

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

private:

  Blocks blocks;
  std::discrete_distribution<> dist;
  const size_t length;

};
