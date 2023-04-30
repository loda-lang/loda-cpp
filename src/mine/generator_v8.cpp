#include "mine/generator_v8.hpp"

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"

GeneratorV8::GeneratorV8(const Config &config, const Stats &stats)
    : Generator(config, stats),
      log_scheduler(60),  // 1 minute
      num_invalid_programs(0) {
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

Program GeneratorV8::readNextProgram() {
  Program program;
  if (!file_in) {
    return program;
  }
  line.clear();
  while (line.empty()) {
    if (!std::getline(*file_in, line)) {
      file_in.reset();  // close file
      return program;
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
    num_invalid_programs++;
    program.ops.clear();
  }
  return program;
}

Program GeneratorV8::generateProgram() {
  Program program;
  while (file_in && program.ops.empty()) {
    program = readNextProgram();
  }
  // log message on invalid programs
  bool log_invalid = false;
  if (log_scheduler.isTargetReached()) {
    log_scheduler.reset();
    log_invalid = true;
  }
  if (!file_in) {
    log_invalid = true;
  }
  if (log_invalid && num_invalid_programs > 0) {
    Log::get().warn("Ignored " + std::to_string(num_invalid_programs) +
                    " invalid programs");
    num_invalid_programs = 0;
  }
  return program;
}

std::pair<Operation, double> GeneratorV8::generateOperation() {
  throw std::runtime_error("unsupported operation");
}

bool GeneratorV8::supportsRestart() const {
  // restart is not supported because we would start reading the file from the
  // beginning again
  return false;
}

bool GeneratorV8::isFinished() const { return !file_in; };
