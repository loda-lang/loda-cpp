#pragma once

#include "number.hpp"
#include "program.hpp"

#include <stack>

class Mutator
{
public:

  void mutateConstants( const Program &program, size_t num_results, std::stack<Program> &result );

};
