#pragma once

#include <vector>

class Assignment
{
public:

  Assignment( uint8_t variable, bool is_delta, int8_t value )
  {
  }

  uint8_t getVariable() const
  {
    return variable;
  }

  bool isDelta() const
  {

  }

  void apply() const
  {

  }

private:

  const uint8_t variable;
  const int8_t value;
};

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
