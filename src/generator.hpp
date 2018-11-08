#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

#include <random>

class State
{
public:

  State();

  State( size_t numStates, size_t maxConstant, size_t num_operation_types, size_t num_target_types,
      size_t num_source_types );

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

  Generator( const Settings& settings, size_t numStates, int64_t seed );

  Generator( const Generator& other ) = default;

  Generator operator+( const Generator& other );

  void mutate( double delta );

  Program generateProgram( size_t initialState = 0 );

  void generateOperations( Seed& seed );

  void setSeed( int64_t seed );

  void print();

  number_t score;

private:

  const Settings& settings;

  std::vector<State> states;

  std::vector<Operation::Type> operation_types;
  std::vector<Operand::Type> source_operand_types;
  std::vector<Operand::Type> target_operand_types;
  Program program_template;

  std::mt19937 gen;

};

inline bool less_than_score( const Generator::UPtr& g1, const Generator::UPtr& g2 )
{
  return (g1->score < g2->score);
}

class Scorer
{
public:

  virtual ~Scorer();

  virtual number_t score( const Sequence& s ) = 0;

};

class FixedSequenceScorer: public Scorer
{
public:

  FixedSequenceScorer( const Sequence& target );

  virtual ~FixedSequenceScorer();

  virtual number_t score( const Sequence& s ) override;

private:
  Sequence target_;

};

class Finder
{
public:

  Finder( const Settings& settings );

  Program find( Scorer& scorer, size_t seed, size_t max_iterations );

private:
  const Settings& settings;

};
