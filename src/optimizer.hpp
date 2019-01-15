#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Optimizer
{
public:

  Optimizer( const Settings& settings )
      : settings( settings )
  {
  }

  void optimize( Program& p, size_t num_reserved_cells, size_t num_initialized_cells );

  void minimize( Program& p, size_t num_terms );

private:

  bool removeNops( Program& p );

  bool removeEmptyLoops( Program& p );

  bool mergeOps( Program& p );

  void simplifyOperands( Program& p, size_t num_initialized_cells );

  bool reduceMemoryCells( Program& p, size_t num_reserved_cells );

  Settings settings;

};
