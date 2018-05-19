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

class Seed
{
public:
  size_t state;
  double position;
  std::vector<Operation::UPtr> ops;
};

class Generator
{
public:

  Generator( size_t numStates, int64_t seed );

  Generator operator+( const Generator& other );

  void mutate( double delta );

  Program::UPtr generateProgram( size_t initialState );

  void generateOperations( Seed& seed );

  void setSeed( int64_t seed );

  std::vector<State> states;

  std::mt19937 gen;

};
