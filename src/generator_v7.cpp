#include "generator_v7.hpp"

#include "file.hpp"
#include "oeis_sequence.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"

GeneratorV7::GeneratorV7(const Config &config, const Stats &stats)
    : Generator(config, stats),
      mutator(stats, config.mutation_rate, true  // mutate comments!
      ) {
  // load patterns
  Parser parser;
  Program program;
  const std::string patterns_home = Setup::getProgramsHome() + "patterns";
  if (isDir(patterns_home)) {
    for (const auto &it : std::filesystem::directory_iterator(patterns_home)) {
      if (it.path().filename().extension().string() != ".asm") {
        continue;
      }
      const auto path = it.path().string();
      try {
        program = parser.parse(path);
        patterns.push_back(program);
      } catch (std::exception &) {
        Log::get().warn("Cannot load pattern " + path);
      }
    }
  }
  if (patterns.empty()) {
    Log::get().error("No patterns found", true);
  } else {
    Log::get().info("Loaded " + std::to_string(patterns.size()) + " patterns");
  }
}

Program GeneratorV7::generateProgram() {
  auto program = patterns[Random::get().gen() % patterns.size()];
  mutator.mutateRandom(program);
  return program;
}

std::pair<Operation, double> GeneratorV7::generateOperation() {
  throw std::runtime_error("unsupported operation");
  return {};
}
