#include "mine/generator_v4.hpp"

#include <math.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "lang/parser.hpp"
#include "lang/program_util.hpp"
#include "mine/generator.hpp"
#include "mine/generator_v1.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

ProgramState::ProgramState() : index(0), generated(0) {}

void ProgramState::validate() const {
  if (index < 1 || index >= 10000) {
    throw std::runtime_error("invalid program state index: " +
                             std::to_string(index));
  }
}

void throwProgramStateLoadError() {
  throw std::runtime_error("program state load error");
}

void ProgramState::load(const std::string& path) {
  validate();
  Parser parser;
  Program p = parser.parse(path);
  size_t step = 0;
  start.ops.clear();
  current.ops.clear();
  end.ops.clear();
  for (auto& op : p.ops) {
    if (op.type == Operation::Type::NOP && !op.comment.empty()) {
      if (op.comment == "start") {
        step = 1;
      } else if (op.comment.rfind("current: ", 0) == 0) {
        step = 2;
        auto sub = op.comment.substr(9);
        generated = stoll(sub);
      } else if (op.comment == "end") {
        step = 3;
      } else {
        throwProgramStateLoadError();
      }
      continue;
    }
    switch (step) {
      case 1:
        start.ops.push_back(op);
        break;
      case 2:
        current.ops.push_back(op);
        break;
      case 3:
        end.ops.push_back(op);
        break;
      default:
        throwProgramStateLoadError();
    }
  }
}

void ProgramState::save(const std::string& path) const {
  validate();
  Program p;
  Operation nop(Operation::Type::NOP);
  nop.comment = "start";
  p.ops.push_back(nop);
  p.ops.insert(p.ops.end(), start.ops.begin(), start.ops.end());
  nop.comment = "current: " + std::to_string(generated);
  p.ops.push_back(nop);
  p.ops.insert(p.ops.end(), current.ops.begin(), current.ops.end());
  nop.comment = "end";
  p.ops.push_back(nop);
  p.ops.insert(p.ops.end(), end.ops.begin(), end.ops.end());
  std::ofstream f(path);
  ProgramUtil::print(p, f);
  f.close();
}

GeneratorV4::GeneratorV4(const Config& config, const Stats& stats)
    : Generator(config, stats),
      scheduler(600)  // 10 minutes (magic number)
{
  if (config.miner.empty() || config.miner == "default") {
    Log::get().error("Invalid or empty miner for generator v4: " + config.miner,
                     true);
  }

  const auto loda_home = Setup::getLodaHome();
  moveDirToParent(loda_home, "gen_v4", "cache");
  home = Setup::getCacheHome() + "gen_v4" + FILE_SEP + config.miner;
  numfiles_path = home + FILE_SEP + "numfiles.txt";

  // obtain lock
  FolderLock lock(home);
  std::ifstream nf(numfiles_path);
  if (!nf.good()) {
    init(stats);
  }
  nf.close();
  load();
}

std::string GeneratorV4::getPath(int64_t index) const {
  std::stringstream s;
  s << home << std::string(1, FILE_SEP) << "S" << std::setw(4)
    << std::setfill('0') << index << ".txt";
  return s.str();
}

void GeneratorV4::init(const Stats& stats) {
  Log::get().info("Initializing state of generator v4 in " + home);

  Generator::Config config;
  config.version = 1;
  config.loops = true;
  config.calls = false;
  config.indirect_access = false;

  std::vector<Program> programs;
  for (int64_t length = 3; length <= 20; length++) {
    int64_t count = pow(1.25, length);
    config.length = length;
    config.max_constant = std::min<int64_t>(length / 4, 2);
    config.max_index = std::min<int64_t>(length / 4, 2);
    GeneratorV1 gen_v1(config, stats);
    for (int64_t c = 0; c < count; c++) {
      programs.push_back(gen_v1.generateProgram());
    }
  }

  std::sort(programs.begin(), programs.end());

  ensureDir(home);

  ProgramState s;
  s.index = 1;
  s.generated = 0;
  s.start.push_back(Operation::Type::MOV, Operand::Type::DIRECT,
                    Program::OUTPUT_CELL, Operand::Type::CONSTANT, 0);
  for (const auto& p : programs) {
    if (p == s.start) {
      continue;
    }
    s.current = s.start;
    s.end = p;
    s.save(getPath(s.index));
    s.start = p;
    s.index++;
  }

  std::ofstream nf(numfiles_path);
  nf << (s.index - 1) << std::endl;
  nf.close();
}

void GeneratorV4::load() {
  std::ifstream nf(numfiles_path);
  if (!nf.good()) {
    Log::get().error("File not found: " + numfiles_path, true);
  }
  int64_t num_files;
  nf >> num_files;
  nf.close();
  if (num_files < 1 || num_files >= 10000) {
    Log::get().error("Invalid number of files: " + std::to_string(num_files),
                     true);
  }
  int64_t attempts = num_files * 100;
  do {
    state = ProgramState();
    state.index = (Random::get().gen() % num_files) + 1;
    state.load(getPath(state.index));
    iterator = Iterator(state.current);
  } while (state.end < state.current && --attempts);
  if (!attempts) {
    Log::get().error("Looks like we already generated all programs!", true);
  }
  Log::get().debug("Working on gen_v4 block " + std::to_string(state.index) +
                   " (" + std::to_string(state.generated) +
                   " generated programs)");
}

Program GeneratorV4::generateProgram() {
  state.current = iterator.next();
  state.generated++;
  if (scheduler.isTargetReached()) {
    scheduler.reset();
    FolderLock lock(home);
    state.save(getPath(state.index));
    load();
  }
  return state.current;
}

std::pair<Operation, double> GeneratorV4::generateOperation() {
  throw std::runtime_error("unsupported operation in generator v4");
}

bool GeneratorV4::supportsRestart() const { return true; }

bool GeneratorV4::isFinished() const { return false; };
