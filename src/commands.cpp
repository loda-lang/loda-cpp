#include "commands.hpp"

#include <iostream>

#include "benchmark.hpp"
#include "boinc.hpp"
#include "evaluator.hpp"
#include "iterator.hpp"
#include "miner.hpp"
#include "minimizer.hpp"
#include "mutator.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "setup.hpp"
#include "test.hpp"
#include "util.hpp"

void Commands::initLog(bool silent) {
  if (silent && Log::get().level != Log::Level::DEBUG) {
    Log::get().silent = true;
  } else {
    Log::get().silent = false;
    Log::get().info("Starting " + Version::INFO +
                    ". See https://loda-lang.org/");
  }
}

void Commands::help() {
  initLog(true);
  Settings settings;
  std::cout << "Welcome to " << Version::INFO
            << ". More information at https://loda-lang.org/" << std::endl
            << std::endl;
  std::cout << "Usage: loda <command> <options>" << std::endl << std::endl;
  std::cout << "Core Commands:" << std::endl;
  std::cout
      << "  evaluate <program>  Evaluate a program to an integer sequence (see "
         "-t,-b,-s)"
      << std::endl;
  std::cout << "  optimize <program>  Optimize a program and print it"
            << std::endl;
  std::cout << "  minimize <program>  Minimize a program and print it (see -t)"
            << std::endl;
  std::cout << "  profile  <program>  Measure program evaluation time (see -t)"
            << std::endl;
  std::cout << "  setup               Run interactive setup to configure LODA"
            << std::endl;

  std::cout << std::endl << "OEIS Commands:" << std::endl;
  std::cout << "  mine                Mine programs for OEIS sequences (see "
               "-i,-p,-P,-H)"
            << std::endl;
  std::cout << "  check <program>     Check a program for an OEIS sequence "
               "(see -b)"
            << std::endl;
  std::cout << "  submit <file> [id]  Submit a program for an OEIS sequence"
            << std::endl;
  std::cout << std::endl << "Targets:" << std::endl;
  std::cout
      << "  <file>              Path to a LODA file (file extension: *.asm)"
      << std::endl;
  std::cout << "  <id>                ID of an OEIS integer sequence "
               "(example: A000045)"
            << std::endl;
  std::cout << "  <program>           Either an <file> or an <id>" << std::endl;

  std::cout << std::endl << "Options:" << std::endl;
  std::cout << "  -t <number>         Number of sequence terms (default: "
            << settings.num_terms << ")" << std::endl;
  std::cout
      << "  -b                  Print result in b-file format from offset 0"
      << std::endl;
  std::cout << "  -B <number>         Print result in b-file format from a "
               "custom offset"
            << std::endl;
  std::cout << "  -s                  Evaluate program to number of "
               "execution steps"
            << std::endl;
  std::cout << "  -c <number>         Maximum number of interpreter cycles "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -m <number>         Maximum number of used memory cells "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -l <string>         Log level (values: "
               "debug,info,warn,error,alert)"
            << std::endl;
  std::cout
      << "  -i <string>         Name of miner configuration from miners.json"
      << std::endl;
  std::cout << "  -p                  Parallel mining using default number of "
               "instances"
            << std::endl;
  std::cout << "  -P <number>         Parallel mining using custom number of "
               "instances"
            << std::endl;
  std::cout
      << "  -H <number>         Number of mining hours (default unlimited)"
      << std::endl;
}

std::pair<std::string, size_t> getProgramPathAndSeqId(std::string arg) {
  std::pair<std::string, size_t> result;
  try {
    OeisSequence s(arg);
    result.first = s.getProgramPath();
    result.second = s.id;
  } catch (...) {
    // not an ID string
    result.first = arg;
    result.second = 0;
  }
  return result;
}

// official commands

void Commands::setup() { Setup::runWizard(); }

void Commands::evaluate(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  Evaluator evaluator(settings);
  Sequence seq;
  evaluator.eval(program, seq);
  if (!settings.print_as_b_file) {
    std::cout << seq << std::endl;
  }
}

void Commands::check(const std::string& path) {
  initLog(true);
  auto path_and_id = getProgramPathAndSeqId(path);
  Parser parser;
  Program program = parser.parse(path_and_id.first);
  OeisSequence seq(path_and_id.second);
  if (seq.id == 0) {
    auto id_str = ProgramUtil::getSequenceIdFromProgram(program);
    seq = OeisSequence(id_str);
  }
  Evaluator evaluator(settings);
  auto terms = seq.getTerms(OeisSequence::FULL_SEQ_LENGTH);
  // don't use incremental evaluation in maintenance to detect bugs
  auto result = evaluator.check(
      program, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id, false);
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
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  Optimizer optimizer(settings);
  optimizer.optimize(program);
  ProgramUtil::print(program, std::cout);
}

void Commands::minimize(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  Minimizer minimizer(settings);
  minimizer.optimizeAndMinimize(program, OeisSequence::DEFAULT_SEQ_LENGTH);
  ProgramUtil::print(program, std::cout);
}

void Commands::profile(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  Sequence res;
  Evaluator evaluator(settings);
  auto start_time = std::chrono::steady_clock::now();
  evaluator.eval(program, res);
  auto cur_time = std::chrono::steady_clock::now();
  auto micro_secs = std::chrono::duration_cast<std::chrono::microseconds>(
                        cur_time - start_time)
                        .count();
  std::cout.setf(std::ios::fixed);
  std::cout.precision(3);
  if (micro_secs < 1000) {
    std::cout << micro_secs << "µs" << std::endl;
  } else if (micro_secs < 1000000) {
    std::cout << static_cast<double>(micro_secs) / 1000.0 << "ms" << std::endl;
  } else {
    std::cout << static_cast<double>(micro_secs) / 1000000.0 << "s"
              << std::endl;
  }
}

void Commands::mine() {
  initLog(false);
  std::unique_ptr<ProgressMonitor> progress_monitor;
  if (settings.num_mine_hours > 0) {
    const int64_t target_seconds = settings.num_mine_hours * 3600;
    progress_monitor.reset(new ProgressMonitor(target_seconds, "", "", 0));
  }
  Miner miner(settings, Miner::DEFAULT_LOG_INTERVAL, progress_monitor.get());
  miner.mine();
}

void Commands::submit(const std::string& path, const std::string& id) {
  initLog(false);
  Miner miner(settings);
  miner.submit(path, id);
}

// hidden commands

void Commands::boinc() {
  initLog(false);
  Boinc boinc(settings);
  boinc.run();
}

void Commands::test() {
  initLog(false);
  Test test;
  test.all();
}

void Commands::testIncEval() {
  initLog(false);
  Settings settings;
  OeisManager manager(settings);
  auto& stats = manager.getStats();
  int64_t count = 0;
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (stats.all_program_ids[id] && Test::checkIncEval(settings, id, false)) {
      count++;
    }
  }
  Log::get().info("Passed incremental evaluation check for " +
                  std::to_string(count) + " programs");
}

void Commands::dot(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  ProgramUtil::exportToDot(program, std::cout);
}

void Commands::generate() {
  initLog(true);
  OeisManager manager(settings);
  MultiGenerator multi_generator(settings, manager.getStats(), false);
  auto program = multi_generator.generateProgram();
  ProgramUtil::print(program, std::cout);
}

void Commands::migrate() {
  initLog(false);
  OeisManager manager(settings);
  manager.migrate();
}

void Commands::maintain(const std::string& id) {
  initLog(false);
  OeisSequence seq(id);
  OeisManager manager(settings);
  manager.load();
  manager.maintainProgram(seq.id);
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

void Commands::benchmark() {
  Benchmark benchmark;
  benchmark.all();
}
