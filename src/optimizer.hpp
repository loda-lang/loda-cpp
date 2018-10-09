#pragma once

#include "number.hpp"
#include "program.hpp"

class Optimizer
{
public:

  void optimize( Program& p, size_t num_initialized_cells );

  void minimize( Program& p, size_t num_terms );

private:

  bool removeNops( Program& p );

  bool removeEmptyLoops( Program& p );

  bool mergeOps( Program& p );

  void simplifyOperands( Program& p, size_t num_initialized_cells );

};
