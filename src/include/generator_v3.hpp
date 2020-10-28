#pragma once

#include "generator.hpp"

struct OpProb
{
  Operation operation;
  size_t partial_sum;
};

class GeneratorV3: public Generator
{
public:

  GeneratorV3( const Settings &settings, int64_t seed );

  virtual Program generateProgram() override;

private:

  std::discrete_distribution<> length_dist;
  std::vector<std::vector<OpProb>> operation_dists;

};
