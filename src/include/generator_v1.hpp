#pragma once

#include "generator.hpp"

class GeneratorV1: public Generator
{
public:

  GeneratorV1( const Config &config, const Stats &stats );

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

private:

  int64_t num_operations;
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
  std::vector<Number> constants;
  Program program_template;

};
