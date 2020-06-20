#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  Generator( const Settings &settings, int64_t seed );

  Generator( const Generator &other ) = default;

  Program generateProgram();

  void setSeed( int64_t seed );

private:

  void generateOperations();

  const Settings &settings;

  std::discrete_distribution<> operation_dist;
  std::discrete_distribution<> target_type_dist;
  std::discrete_distribution<> target_value_dist;
  std::discrete_distribution<> source_type_dist;
  std::discrete_distribution<> source_value_dist;
  std::discrete_distribution<> position_dist;
  std::discrete_distribution<> constants_dist;

  std::vector<Operation::Type> operation_types;
  std::vector<Operand::Type> source_operand_types;
  std::vector<Operand::Type> target_operand_types;
  std::vector<number_t> constants;
  Program program_template;

  std::vector<Operation> next_ops;
  double next_position;

  std::mt19937 gen;

};
