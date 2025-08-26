#include "mine/generator_v7.hpp"

#include "lang/comments.hpp"
#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "seq/managed_seq.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

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
      Operation dummy(Operation::Type::NOP);
      dummy.comment = "dummy";
      try {
        program = parser.parse(path);
        ProgramUtil::removeOps(program, Operation::Type::NOP);
        bool has_comment = false;
        for (auto &op : program.ops) {
          has_comment = has_comment || !op.comment.empty();
        }
        if (has_comment) {
          // add dummy comments at begin and end of program
          program.ops.insert(program.ops.begin(), dummy);
          program.ops.push_back(dummy);
          patterns.push_back(program);
        } else {
          Log::get().warn("Missing annotations in pattern " +
                          it.path().filename().string());
        }
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

  // std::cout << "==== BEGIN PATTERN ======" << std::endl;
  // ProgramUtil::print(program, std::cout);
  // std::cout << "==== END PATTERN ======" << std::endl << std::endl;

  mutator.mutateRandom(program);
  ProgramUtil::removeOps(program, Operation::Type::NOP);
  Comments::removeComments(program);

  return program;
}

std::pair<Operation, double> GeneratorV7::generateOperation() {
  throw std::runtime_error("unsupported operation");
}

bool GeneratorV7::supportsRestart() const { return true; }

bool GeneratorV7::isFinished() const { return false; };
