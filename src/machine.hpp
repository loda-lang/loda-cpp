#pragma once

#include "distribution.hpp"

class State
{
public:

  State( Value numStates )
      : transDist( numStates )
  {
  }

  State()
      : State( 0 )
  {
  }

  OperationDistribution operationDist;
  OperandDistribution targetOpDist;
  OperandDistribution sourceOpDist;
  NormalDistribution targetValDist;
  NormalDistribution sourceValDist;
  TransitionDistribution transDist;
  NormalDistribution posDist;

  State operator+( const State& o );

};

class Machine
{
public:

  Machine( Value numStates, int64_t maxOperations );

  Machine operator+( const Machine& o );

  std::vector<State> states;
  int64_t maxOperations;

};
