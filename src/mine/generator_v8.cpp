#include "mine/generator_v8.hpp"

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"

GeneratorV8::GeneratorV8(const Config &config, const Stats &stats)
    : Generator(config, stats) {
  // open file
  if (config.batch_file.empty()) {
    Log::get().error("Missing batch file in generator config", true);
  }
  file_in.reset(new std::ifstream(config.batch_file));
  if (!file_in->good()) {
    Log::get().error("Error opening batch file: " + config.batch_file, true);
  }
  Log::get().info("Reading programs from batch file \"" + config.batch_file +
                  "\"");
}

Program GeneratorV8::generateProgram() {
  Program program;
  while (program.ops.empty()) {
    line.clear();
    while (line.empty()) {
      if (!std::getline(*file_in, line)) {
        Log::get().error("Reached end of file", true);
      }
    }
    std::replace(line.begin(), line.end(), ';', '\n');
    std::stringstream buf(line);
    try {
      program = parser.parse(buf);
      ProgramUtil::removeOps(program, Operation::Type::NOP);
      ProgramUtil::validate(program);
    } catch (std::exception &) {
      // invalid program => skip
      program.ops.clear();
    }
  }
  return program;
}

std::pair<Operation, double> GeneratorV8::generateOperation() {
  throw std::runtime_error("unsupported operation");
}
