#pragma once

#include "assignment.hpp"

class Program;

class Operation
{
  std::vector<Assignment> assigments;
  std::vector<uint8_t> loop_variables;
  std::unique_ptr<Program> loop_program;
};

class Program
{
  std::vector<Operation> operations;
};
