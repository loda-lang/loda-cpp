#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  Generator( const Settings &settings, int64_t seed );

  virtual ~Generator()
  {
  }

  virtual Program generateProgram() = 0;

protected:

  std::vector<number_t> fixCausality( Program &p );

  void ensureSourceNotOverwritten( Program &p );

  void ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells );

  void ensureMeaningfulLoops( Program &p );

  const Settings &settings;
  std::mt19937 gen;

};
