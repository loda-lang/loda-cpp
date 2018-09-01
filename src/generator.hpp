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

  void print();

};

class Seed
{
public:
  size_t state;
  double position;
  std::vector<Operation> ops;
};

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  Generator( size_t numStates, int64_t seed );

  Generator( const Generator& other ) = default;

  Generator operator+( const Generator& other );

  void mutate( double delta );

  Program generateProgram( size_t initialState );

  void generateOperations( Seed& seed );

  void setSeed( int64_t seed );

  void print();

  std::vector<State> states;

  std::mt19937 gen;

  Value score;

};

inline bool less_than_score( const Generator::UPtr& g1, const Generator::UPtr& g2 )
{
  return (g1->score < g2->score);
}
