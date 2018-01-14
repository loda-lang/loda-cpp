#pragma once

#include "program.hpp"
#include "sequence.hpp"

class Evaluator
{
public:

  Sequence Eval( Program& p, Value length );

};
