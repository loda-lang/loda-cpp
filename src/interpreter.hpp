#pragma once

#include "program.hpp"
#include "memory.hpp"

class Interpreter
{
public:
  bool Run( Program& p, Memory& mem );

};
