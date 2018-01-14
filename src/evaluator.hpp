#pragma once

#include "program.hpp"
#include "memory.hpp"

class Evaluator
{
public:

  Memory Eval( Program& p, Value length );

};
