#pragma once

#include "common.hpp"
#include "program.hpp"

class Optimizer
{
public:

  void optimize( Program& p );

  bool removeNops( Program& p );

  bool removeEmptyLoops( Program& p );

};
