#pragma once

#include "lang/parser.hpp"
#include "mine/generator.hpp"
#include "mine/mutator.hpp"
#include "sys/util.hpp"

/*
 * Generator that loads programs from a batch file.
 * In the btach file, every line corresponds to one
 * program where the operations are separated using
 * semicolons. This generator reads the programs
 * from the batch file and passes them to the miner.
 */
class GeneratorV8 : public Generator {
 public:
  GeneratorV8(const Config &config, const Stats &stats);

  virtual Program generateProgram() override;

  virtual std::pair<Operation, double> generateOperation() override;

 private:
  std::shared_ptr<std::ifstream> file_in;
  Parser parser;
  std::string line;
  AdaptiveScheduler log_scheduler;
  size_t num_invalid_programs;

  Program readNextProgram();
};
