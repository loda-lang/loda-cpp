#pragma once

#include "program.hpp"
#include "sequence.hpp"

#define DEFAULT_RATE 1000

class OperandDistribution
{
public:

  OperandDistribution()
      : constantRate( DEFAULT_RATE ),
        memAccessDirectRate( DEFAULT_RATE ),
        memAccessIndirectRate( DEFAULT_RATE )
  {
  }

  OperandDistribution operator+( const OperandDistribution& o )
  {
    OperandDistribution r;
    r.constantRate = constantRate + o.constantRate;
    r.memAccessDirectRate = memAccessDirectRate + o.memAccessDirectRate;
    r.memAccessIndirectRate = memAccessIndirectRate + o.memAccessIndirectRate;
    auto sum = r.constantRate + r.memAccessDirectRate + r.memAccessIndirectRate;
    r.constantRate = r.constantRate * DEFAULT_RATE / sum;
    r.memAccessDirectRate = r.memAccessDirectRate * DEFAULT_RATE / sum;
    r.memAccessIndirectRate = r.memAccessIndirectRate * DEFAULT_RATE / sum;
    return r;
  }

  Value constantRate;
  Value memAccessDirectRate;
  Value memAccessIndirectRate;

};

class OperationDistribution
{
public:
  OperationDistribution()
      : movRate( DEFAULT_RATE ),
        addRate( DEFAULT_RATE ),
        subRate( DEFAULT_RATE ),
        loopRate( DEFAULT_RATE )
  {
  }

  OperationDistribution operator+( const OperationDistribution& o )
  {
    OperationDistribution r;
    r.movRate = movRate + o.movRate;
    r.addRate = addRate + o.addRate;
    r.subRate = subRate + o.subRate;
    r.loopRate = loopRate + o.loopRate;
    auto sum = r.movRate + r.addRate + r.subRate + r.loopRate;
    r.movRate = r.movRate * DEFAULT_RATE / sum;
    r.addRate = r.addRate * DEFAULT_RATE / sum;
    r.subRate = r.subRate * DEFAULT_RATE / sum;
    r.loopRate = r.loopRate * DEFAULT_RATE / sum;
    return r;
  }

  Value movRate;
  Value addRate;
  Value subRate;
  Value loopRate;

};

class TransitionDistribution
{
public:
  TransitionDistribution( Value numStates )
  {
    rates.data.resize( numStates );
    for ( Value state = 0; state < numStates; state++ )
    {
      rates.data[state] = DEFAULT_RATE;
    }
  }

  TransitionDistribution()
      : TransitionDistribution( 0 )
  {
  }

  TransitionDistribution operator+( const TransitionDistribution& o )
  {
    TransitionDistribution r;
    r.rates.data.resize( rates.data.size() );
    Value sum = 0;
    for ( Value i = 0; i < rates.Length(); i++ )
    {
      r.rates.data[i] = rates.data[i] + o.rates.data[i];
      sum += r.rates.data[i];
    }
    for ( Value i = 0; i < rates.Length(); i++ )
    {
      r.rates.data[i] = r.rates.data[i] * DEFAULT_RATE / sum;
    }
    return r;
  }

  Sequence rates;

};

// used for position of new operations w.r.t. program length
class NormalDistribution
{
public:
  NormalDistribution()
      : mean( 0.5 ),
        stddev( 5.0 )
  {
  }
  double mean;
  double stddev;

  NormalDistribution operator+( const NormalDistribution& o )
  {
    NormalDistribution r;
    r.mean = (mean + o.mean) / 2.0;
    r.stddev = (stddev + o.stddev) / 2.0;
    return r;
  }

};

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
  OperandDistribution targetDist;
  OperandDistribution sourceDist;
  TransitionDistribution transDist;
  NormalDistribution posDist;

  State operator+( const State& o )
  {
    State r;
    r.operationDist = operationDist + o.operationDist;
    r.targetDist = targetDist + o.targetDist;
    r.sourceDist = sourceDist + o.sourceDist;
    r.transDist = transDist + o.transDist;
    r.posDist = posDist + o.posDist;
    return r;
  }

};

class Machine
{
public:

  Machine( Value numStates )
  {
    states.resize( numStates );
    for ( Value state = 0; state < numStates; state++ )
    {
      states[state] = State( numStates );
    }
  }

  Machine operator+( const Machine& o )
  {
    Machine r( states.size() );
    for ( Value s = 0; s < states.size(); s++ )
    {
      r.states[s] = states[s] + o.states[s];
    }
    return r;
  }

  std::vector<State> states;

};
