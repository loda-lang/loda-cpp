#pragma once

#include "mine/generator.hpp"
#include "mine/mutator.hpp"

class GeneratorV1 : public Generator {
 public:
  GeneratorV1(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

  virtual bool supportsRestart() const override;

  virtual bool isFinished() const override;

 private:
  int64_t num_operations;
  size_t current_template;
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
  std::vector<Program> templates;
  Mutator mutator;
};
