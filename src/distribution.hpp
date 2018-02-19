#pragma once

#include "program.hpp"
#include "sequence.hpp"

#include <random>

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

  OperandDistribution operator+( const OperandDistribution& o );

  Operand operator()( std::default_random_engine& engine );

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

  OperationDistribution operator+( const OperationDistribution& o );

  Value movRate;
  Value addRate;
  Value subRate;
  Value loopRate;

};

class TransitionDistribution
{
public:
  TransitionDistribution( Value numStates );

  TransitionDistribution()
      : TransitionDistribution( 0 )
  {
  }

  TransitionDistribution operator+( const TransitionDistribution& o );

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

  NormalDistribution operator+( const NormalDistribution& o );

};
