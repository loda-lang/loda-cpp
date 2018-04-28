#pragma once

#include "program.hpp"

class Optimizer
{
public:

  void Optimize( Program& p );

  size_t RemoveEmptyLoops( Program& p );

};
