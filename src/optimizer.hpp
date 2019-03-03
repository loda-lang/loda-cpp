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

  void optimize( Program& p, size_t num_reserved_cells, size_t num_initialized_cells ) const;

  void minimize( Program& p, size_t num_terms ) const;

private:

  bool removeNops( Program& p ) const;

  bool removeEmptyLoops( Program& p ) const;

  bool mergeOps( Program& p ) const;

  void simplifyOperands( Program& p, size_t num_initialized_cells ) const;

  bool reduceMemoryCells( Program& p, size_t num_reserved_cells ) const;

  Settings settings;

};
