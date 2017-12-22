#pragma once

#include "assignment.hpp"

#include <tuple>
#include <vector>

template<size_t NUM_VARS>
class Program
{
public:

  using Ptr = std::unique_ptr<Program<NUM_VARS> >;

  class Loop
  {
  public:
    std::vector<size_t> vars;
    Ptr body;
  };

  class Operation
  {
  public:

    Assignment::Array<NUM_VARS> assignments;
    Loop loop;

  };

  std::vector<Operation> operations;

};
