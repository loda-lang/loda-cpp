#include "generator_v6.hpp"

#include "oeis_sequence.hpp"
#include "parser.hpp"
#include "program_util.hpp"

GeneratorV6::GeneratorV6(const Config &config, const Stats &stats)
    : Generator(config, stats),
      scheduler(120),  // 2 minutes
      mutator(stats) {
  // get first program template
  nextProgram();
}

Program GeneratorV6::generateProgram() {
  if (scheduler.isTargetReached()) {
    scheduler.reset();
    nextProgram();
  }
  Program result(program);
  mutator.mutateRandom(result);
  return result;
}

void GeneratorV6::nextProgram() {
  Parser parser;
  for (int64_t i = 0; i < 10; i++) {
    const auto id = getRandomProgramId();
    const std::string path = OeisSequence(id).getProgramPath();
    try {
      program = parser.parse(path);
      ProgramUtil::removeOps(program, Operation::Type::NOP);
      // Log::get().debug("Loaded template: " + path);
      return;
    } catch (std::exception &) {
      Log::get().warn("Cannot load program " + path);
    }
  }
  Log::get().error("Error loading template for generator v6", true);
}

std::pair<Operation, double> GeneratorV6::generateOperation() {
  throw std::runtime_error("unsupported operation");
  return {};
}
