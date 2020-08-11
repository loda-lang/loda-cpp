#pragma once

#include "program.hpp"
#include "util.hpp"

class Minimizer
{
public:

  Minimizer( const Settings &settings )
      :
      settings( settings )
  {
  }

  void minimize( Program &p, size_t num_terms ) const;

  void optimizeAndMinimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells,
      size_t num_terms ) const;

private:

  Settings settings;

};
