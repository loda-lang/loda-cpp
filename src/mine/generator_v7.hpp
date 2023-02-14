#pragma once

#include "mine/generator.hpp"
#include "mine/mutator.hpp"

/*
 * Generator that uses pattern files. A pattern is a program with annotation
 * that indicate positions where mutations should be applied.
 */
class GeneratorV7 : public Generator {
 public:
  GeneratorV7(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

 private:
  std::vector<Program> patterns;
  Mutator mutator;
};
