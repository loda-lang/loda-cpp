#pragma once

#include "assignment.hpp"

#include <tuple>
#include <vector>

using var_t = std::size_t;

template<size_t NUM_VARS>
class Program
{
  using Assignments = Assignment::Array<NUM_VARS>;
  using LoopVariables = std::vector<var_t>;
  using ProgramPtr = std::unique_ptr<Program<NUM_VARS> >;
  using Operation = std::tuple<Assignments, LoopVariables, ProgramPtr >;

  std::vector<Operation> operations;
};
