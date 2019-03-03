#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class State
{
public:

  State();

  State( size_t num_states, size_t max_constant, size_t num_operation_types, size_t num_target_types,
      size_t num_source_types );

  Distribution operation_dist;
  Distribution target_type_dist;
  Distribution target_value_dist;
  Distribution source_type_dist;
  Distribution source_value_dist;
  Distribution transition_dist;
  Distribution position_dist;

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
