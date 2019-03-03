#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

#include <unordered_set>

class Optimizer
{
public:

  Optimizer( const Settings& settings )
      : settings( settings )
  {
  }

  void optimize( Program& p, size_t num_reserved_cells, size_t num_initialized_cells ) const;

  void minimize( Program& p, size_t num_terms ) const;

  bool addPostLinear( Program& p, number_t slope, number_t offset );

private:

  bool removeNops( Program& p ) const;

  bool removeEmptyLoops( Program& p ) const;

  bool mergeOps( Program& p ) const;

  void simplifyOperands( Program& p, size_t num_initialized_cells ) const;

  bool reduceMemoryCells( Program& p, size_t num_reserved_cells ) const;

  bool getUsedMemoryCells( const Program& p, std::unordered_set<number_t>& used_cells, number_t& larged_used ) const;

  Settings settings;

};
