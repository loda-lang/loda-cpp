#include "commands.hpp"

#include <iostream>

#include "evaluator.hpp"
#include "iterator.hpp"
#include "miner.hpp"
#include "minimizer.hpp"
#include "mutator.hpp"
#include "oeis_maintenance.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "test.hpp"
#include "util.hpp"

std::string Commands::getVersionInfo() {
#ifdef LODA_VERSION
  return "LODA v" + std::string(xstr(LODA_VERSION)) +
         ", see https://loda-lang.org";
#else
  return "LODA development version, see https://loda-lang.org";
#endif
}

void Commands::initLog(bool silent) {
  if (silent) {
    Log::get().silent = true;
  } else {
    Log::get().silent = false;
    Log::get().info("Starting " + getVersionInfo());
  }
}

void Commands::help() {
  initLog(true);
  Settings settings;
  std::cout << getVersionInfo() << std::endl << std::endl;
  std::cout << "Usage:                   loda <command> <options>" << std::endl;
  std::cout << std::endl << "=== Core commands ===" << std::endl;
  std::cout << "  evaluate <file/seq-id> Evaluate a program to a sequence (cf. "
               "-t,-b,-s)"
            << std::endl;
  std::cout
      << "  minimize <file>        Minimize a program and print it (cf. -t)"
      << std::endl;
  std::cout << "  optimize <file>        Optimize a program and print it"
            << std::endl;
  std::cout << "  setup            Run interactive setup" << std::endl;
  std::cout << std::endl << "=== OEIS commands ===" << std::endl;
  std::cout
      << "  mine                   Mine programs for OEIS sequences (cf. -i)"
      << std::endl;
  std::cout
      << "  match <file>           Match a program to OEIS sequences (cf. -i)"
      << std::endl;
  std::cout << "  check <seq-id>         Check a program for an OEIS sequence "
               "(cf. -b)"
            << std::endl;
  std::cout
      << "  maintain               Maintain all programs for OEIS sequences"
      << std::endl;
  std::cout << std::endl << "=== Command-line options ===" << std::endl;
  std::cout << "  -t <number>            Number of sequence terms (default: "
            << settings.num_terms << ")" << std::endl;
  std::cout << "  -b <number>            Print result in b-file format from a "
               "given offset"
            << std::endl;
  std::cout << "  -s                     Evaluate program to number of "
               "execution steps"
            << std::endl;
  std::cout << "  -c <number>            Maximum number of interpreter cycles "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -m <number>            Maximum number of used memory cells "
               "(no limit: -1)"
            << std::endl;
  std::cout
      << "  -p <number>            Maximum physical memory in MB (default: "
      << settings.max_physical_memory / (1024 * 1024) << ")" << std::endl;
  std::cout << "  -l <string>            Log level (values: "
               "debug,info,warn,error,alert)"
            << std::endl;
  std::cout
      << "  -k <string>            Configuration file (default: loda.json)"
      << std::endl;
  std::cout
      << "  -i <string>            Name of miner configuration from loda.json"
      << std::endl;
  std::cout << std::endl << "=== Environment variables ===" << std::endl;
  std::cout << "LODA_PROGRAMS_HOME       Directory for mined programs "
               "(default: programs)"
            << std::endl;
  std::cout << "LODA_UPDATE_INTERVAL     Update interval for OEIS metadata in "
               "days (default: " +
                   std::to_string(settings.update_interval_in_days) + ")"
            << std::endl;
  std::cout << "LODA_MAX_CYCLES          Maximum number of interpreter cycles "
               "(same as -c)"
            << std::endl;
  std::cout << "LODA_MAX_MEMORY          Maximum number of used memory cells "
               "(same as -m)"
            << std::endl;
  std::cout
      << "LODA_MAX_PHYSICAL_MEMORY Maximum physical memory in MB (same as -p)"
      << std::endl;
  std::cout
      << "LODA_SLACK_ALERTS        Enable alerts on Slack (default: false)"
      << std::endl;
  std::cout
      << "LODA_TWEET_ALERTS        Enable alerts on Twitter (default: false)"
      << std::endl;
  std::cout << "LODA_INFLUXDB_HOST       InfluxDB host name (URL) for "
               "publishing metrics"
            << std::endl;
  std::cout << "LODA_INFLUXDB_AUTH       InfluxDB authentication info "
               "('user:password' format)"
            << std::endl;
}

std::string get_program_path(std::string arg) {
  try {
    OeisSequence s(arg);
    return s.getProgramPath();
  } catch (...) {
    // not an ID string
  }
  return arg;
}

// official commands

void Commands::setup() { Setup::runWizard(); }

void Commands::evaluate(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(get_program_path(path));
  Evaluator evaluator(settings);
  Sequence seq;
  evaluator.eval(program, seq);
  if (!settings.print_as_b_file) {
    std::cout << seq << std::endl;
  }
}

void Commands::check(const std::string& id) {
  initLog(true);
  OeisSequence seq(id);
  Parser parser;
  Program program = parser.parse(seq.getProgramPath());
  Evaluator evaluator(settings);
  auto terms = seq.getTerms(100000);  // magic number
  auto result =
      evaluator.check(program, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
  switch (result.first) {
    case status_t::OK:
      std::cout << "ok" << std::endl;
      break;
    case status_t::WARNING:
      std::cout << "warning" << std::endl;
      break;
    case status_t::ERROR:
      std::cout << "error" << std::endl;
      break;
  }
}

void Commands::optimize(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(get_program_path(path));
  Optimizer optimizer(settings);
  optimizer.optimize(program, 2, 1);
  ProgramUtil::print(program, std::cout);
}

void Commands::minimize(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(get_program_path(path));
  Minimizer minimizer(settings);
  minimizer.optimizeAndMinimize(program, 2, 1,
                                OeisSequence::DEFAULT_SEQ_LENGTH);
  ProgramUtil::print(program, std::cout);
}

void Commands::match(const std::string& path) {
  initLog(false);
  Parser parser;
  Program program = parser.parse(get_program_path(path));
  OeisManager manager(settings);
  Mutator mutator;
  manager.load();
  Sequence norm_seq;
  std::stack<Program> progs;
  progs.push(program);
  size_t new_ = 0, updated = 0;
  // TODO: unify this code with Miner?
  while (!progs.empty()) {
    program = progs.top();
    progs.pop();
    auto seq_programs = manager.getFinder().findSequence(
        program, norm_seq, manager.getSequences());
    for (auto s : seq_programs) {
      auto r = manager.updateProgram(s.first, s.second);
      if (r.first) {
        if (r.second) {
          new_++;
        } else {
          updated++;
        }
        if (progs.size() < 1000 || settings.hasMemory()) {
          mutator.mutateConstants(s.second, 10, progs);
        }
      }
    }
  }
  Log::get().info("Match result: " + std::to_string(new_) + " new programs, " +
                  std::to_string(updated) + " updated");
}

void Commands::mine() {
  initLog(false);
  Miner miner(settings);
  miner.mine();
}

void Commands::maintain() {
  initLog(false);
  OeisMaintenance maintenance(settings);
  maintenance.maintain();
}

// hidden commands (only in development versions)

void Commands::test() {
  initLog(false);
  Test test;
  test.all();
}

void Commands::dot(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(get_program_path(path));
  ProgramUtil::exportToDot(program, std::cout);
}

void Commands::generate() {
  initLog(true);
  OeisManager manager(settings);
  MultiGenerator multi_generator(settings, manager.getStats(), false);
  auto program = multi_generator.getGenerator()->generateProgram();
  ProgramUtil::print(program, std::cout);
}

void Commands::migrate() {
  initLog(false);
  OeisManager manager(settings);
  manager.migrate();
}

void Commands::iterate(const std::string& count) {
  initLog(true);
  int64_t c = stoll(count);
  Iterator it;
  Program p;
  while (c-- > 0) {
    p = it.next();
    //        std::cout << "\x1B[2J\x1B[H";
    ProgramUtil::print(p, std::cout);
    std::cout << std::endl;
    //        std::cin.ignore();
  }
}

void Commands::collatz(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(path);
  Evaluator evaluator(settings);
  Sequence seq;
  evaluator.eval(program, seq);
  bool is_collatz = Miner::isCollatzValuation(seq);
  std::cout << (is_collatz ? "true" : "false") << std::endl;
}
