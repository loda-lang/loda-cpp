#pragma once

#include "assignment.hpp"

#include <tuple>
#include <vector>

template<size_t NUM_VARS>
class Program
{
public:

  class Loop
  {
  public:
    std::vector<size_t> vars;
    std::unique_ptr<Program<NUM_VARS> > body;
  };

  class Operation
  {
  public:

    std::array<Assignment, NUM_VARS> assignments;
    Loop loop;

  };

  std::vector<Operation> operations;

};
