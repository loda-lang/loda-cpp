#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  Generator( const Settings &settings, int64_t seed )
      :
      settings( settings )
  {
    gen.seed( seed );
  }

  virtual ~Generator()
  {
  }

  virtual Program generateProgram() = 0;

protected:

  const Settings &settings;
  std::mt19937 gen;

};
