#pragma once

#include "generator.hpp"

class GeneratorV3: public Generator
{
public:

  GeneratorV3( const Settings &settings, int64_t seed );

  virtual Program generateProgram() override;

private:

  std::discrete_distribution<> length_dist;

};
