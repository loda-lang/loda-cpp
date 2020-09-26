#pragma once

#include "number.hpp"
#include "program.hpp"

#include <stack>

class Mutator
{
public:

  Mutator( int64_t seed );

  void mutateConstants( const Program &program, size_t num_results, std::stack<Program> &result );

private:

  std::mt19937 gen;

};
