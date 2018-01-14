#pragma once

#include "program.hpp"
#include "sequence.hpp"

#define DEFAULT_RATE 1000

class SourceOperandDistribution
{
public:

  SourceOperandDistribution()
      : constantRate( DEFAULT_RATE ),
        memAccessDirectRate( DEFAULT_RATE ),
        memAccessIndirectRate( DEFAULT_RATE )
  {
  }

  Value constantRate;
  Value memAccessDirectRate;
  Value memAccessIndirectRate;

};

class TargetOperandDistribution
{
public:

  TargetOperandDistribution()
      : memAccessDirectRate( DEFAULT_RATE ),
        memAccessIndirectRate( DEFAULT_RATE )
  {
  }

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

  Sequence rates;

};

// used for position of new operations w.r.t. program length
class NormalDistribution
{
public:
  NormalDistribution( double mean, double stddev )
      : mean( mean ),
        stddev( stddev )
  {
  }
  double mean;
  double stddev;

};
