#pragma once

#include "generator.hpp"

class GeneratorV2: public Generator
{
public:

  GeneratorV2( int64_t seed );

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

private:

  std::discrete_distribution<> length_dist;
  std::discrete_distribution<> operation_dist;
  std::vector<Operation> operations;

};
