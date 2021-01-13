#pragma once

#include "interpreter.hpp"
#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

#include <unordered_set>

class Optimizer
{
public:

  Optimizer( const Settings &settings )
      :
      settings( settings ),
      interpreter( settings )
  {
  }

  void optimize( Program &p, size_t num_reserved_cells, size_t num_initialized_cells ) const;

  bool removeNops( Program &p ) const;

  bool removeEmptyLoops( Program &p ) const;

  bool mergeOps( Program &p ) const;

  bool simplifyOperations( Program &p, size_t num_initialized_cells ) const;

  bool reduceMemoryCells( Program &p, size_t num_reserved_cells ) const;

  bool getUsedMemoryCells( const Program &p, std::unordered_set<number_t> &used_cells, number_t &larged_used ) const;

  bool canChangeVariableOrder( const Program &p ) const;

  bool partialEval( Program &p, size_t num_initialized_cells ) const;

  bool shouldSwapOperations( const Operation &first, const Operation &second ) const;

  bool sortOperations( Program &p ) const;

  bool mergeLoops( Program &p ) const;

private:

  Settings settings;
  Interpreter interpreter;

};
