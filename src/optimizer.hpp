#pragma once

#include "number.hpp"
#include "program.hpp"

class Optimizer
{
public:

  void optimize( Program& p );

  bool removeNops( Program& p );

  bool removeEmptyLoops( Program& p );

  void minimize( Program& p, size_t num_terms );

};
