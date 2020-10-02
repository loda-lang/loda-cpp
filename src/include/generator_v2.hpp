#pragma once

#include "generator.hpp"

class GeneratorV2: public Generator
{
public:

  GeneratorV2( const Settings &settings, int64_t seed );

  virtual Program generateProgram() override;

protected:

  virtual std::pair<Operation, double> generateOperation() override;

};
