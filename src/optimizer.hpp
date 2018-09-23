#pragma once

#include "program.hpp"

class Optimizer
{
public:

  void optimize( Program& p );

  number_t removeEmptyLoops( Program& p );

};
