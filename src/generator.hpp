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

  void mutate( double delta );

  Program generateProgram();

  void setSeed( int64_t seed );

private:

  void generateOperations();

  const Settings &settings;

  Distribution operation_dist;
  Distribution target_type_dist;
  Distribution target_value_dist;
  Distribution source_type_dist;
  Distribution source_value_dist;
  Distribution position_dist;

  std::vector<Operation::Type> operation_types;
  std::vector<Operand::Type> source_operand_types;
  std::vector<Operand::Type> target_operand_types;
  Program program_template;

  std::vector<Operation> next_ops;
  double next_position;

  std::mt19937 gen;

};
