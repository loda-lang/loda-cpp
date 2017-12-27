#pragma once

#include "program.hpp"
#include "memory.hpp"

class Interpreter
{
public:

  bool Run( Program& p, Memory& mem );

  value_t Eval( Argument a, Memory& mem );

};
