#pragma once

#include "program.hpp"
#include "sequence.hpp"

#include <random>

class State
{
public:

  State();

  State( size_t numStates );

  std::discrete_distribution<> operationDist;
  std::discrete_distribution<> targetTypeDist;
  std::discrete_distribution<> targetValueDist;
  std::discrete_distribution<> sourceTypeDist;
  std::discrete_distribution<> sourceValueDist;
  std::discrete_distribution<> transitionDist;
  std::discrete_distribution<> positionDist;

  State operator+( const State& other );

};

class Machine
{
public:

  Machine( size_t numStates, int64_t randomSeed );

  Machine operator+( const Machine& other );

  Program::UPtr generateProgram( size_t initialState );

  std::pair<std::vector<Operation::UPtr>, size_t> generateOperations( size_t state );

  std::vector<State> states;

  std::mt19937 gen;

};
