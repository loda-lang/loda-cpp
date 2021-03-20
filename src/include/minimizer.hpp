#pragma once

#include "program.hpp"
#include "util.hpp"

class Minimizer
{
public:

  Minimizer( const Settings &settings )
      : settings( settings )
  {
  }

  bool minimize( Program &p, size_t num_terms ) const;

  bool optimizeAndMinimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells,
      size_t num_terms ) const;

private:

  Settings settings;

};
