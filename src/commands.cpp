#include "commands.hpp"

#include <fstream>
#include <iostream>

#include "benchmark.hpp"
#include "boinc.hpp"
#include "evaluator.hpp"
#include "evaluator_inc.hpp"
#include "evaluator_log.hpp"
#include "formula_gen.hpp"
#include "iterator.hpp"
#include "log.hpp"
#include "miner.hpp"
#include "minimizer.hpp"
#include "mutator.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "pari.hpp"
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
  std::cout << "  export   <program>  Export a program print result (see -o)"
            << std::endl;
  std::cout << "  optimize <program>  Optimize a program and print it"
            << std::endl;
  std::cout << "  minimize <program>  Minimize a program and print it (see -t)"
            << std::endl;
  std::cout << "  profile  <program>  Measure program evaluation time (see -t)"
            << std::endl;

  std::cout << std::endl << "OEIS Commands:" << std::endl;
  std::cout << "  mine                Mine programs for OEIS sequences (see "
               "-i,-p,-P,-H)"
            << std::endl;
  std::cout << "  check <program>     Check a program for an OEIS sequence "
               "(see -b)"
            << std::endl;
  std::cout
      << "  mutate <program>    Mutate a program and mine for OEIS sequences"
      << std::endl;
  std::cout << "  submit <file> [id]  Submit a program for an OEIS sequence"
            << std::endl;

  std::cout << std::endl << "Admin Commands:" << std::endl;
  std::cout << "  setup               Run interactive setup to configure LODA"
            << std::endl;
  std::cout
      << "  update              Run non-interactive update of LODA and its data"
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
  std::cout << "  -o <string>         Export format (formula,loda,pari)"
            << std::endl;
  std::cout
      << "  -d                  Export with dependencies to other programs"
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
      << "  -H <number>         Number of mining hours (default: unlimited)"
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

void Commands::setup() {
  initLog(true);
  Setup::runWizard();
}

void Commands::update() {
  initLog(false);
  OeisManager manager(settings);
  manager.update(true);
  manager.getStats();
  manager.generateLists();
}

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
  minimizer.optimizeAndMinimize(program, settings.num_terms);
  ProgramUtil::print(program, std::cout);
}

void Commands::export_(const std::string& path) {
  initLog(true);
  Parser parser;
  Program program = parser.parse(getProgramPathAndSeqId(path).first);
  const auto& format = settings.export_format;
  Formula formula;
  FormulaGenerator generator;
  if (format.empty() || format == "formula") {
    if (!generator.generate(program, -1, formula, settings.with_deps)) {
      throw std::runtime_error("program cannot be converted to formula");
    }
    std::cout << formula.toString() << std::endl;
  } else if (format == "pari") {
    if (!generator.generate(program, -1, formula, settings.with_deps) ||
        !Pari::convertToPari(formula)) {
      throw std::runtime_error("program cannot be converted to pari");
    }
    std::cout << Pari::toString(formula) << std::endl;
  } else if (format == "loda") {
    ProgramUtil::print(program, std::cout);
  } else {
    throw std::runtime_error("unknown format");
  }
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

std::unique_ptr<ProgressMonitor> makeProgressMonitor(const Settings& settings) {
  std::unique_ptr<ProgressMonitor> progress_monitor;
  if (settings.num_mine_hours > 0) {
    const int64_t target_seconds = settings.num_mine_hours * 3600;
    progress_monitor.reset(new ProgressMonitor(target_seconds, "", "", 0));
  }
  return progress_monitor;
}

void Commands::mine() {
  initLog(false);
  auto progress_monitor = makeProgressMonitor(settings);
  Miner miner(settings, progress_monitor.get());
  miner.mine();
}

void Commands::mutate(const std::string& path) {
  initLog(false);
  Parser parser;
  Program base_program = parser.parse(getProgramPathAndSeqId(path).first);
  auto progress_monitor = makeProgressMonitor(settings);
  Miner miner(settings, progress_monitor.get());
  miner.setBaseProgram(base_program);
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

void Commands::testIncEval(const std::string& test_id) {
  initLog(false);
  Settings settings;
  OeisManager manager(settings);
  auto& stats = manager.getStats();
  size_t target_id = 0;
  if (!test_id.empty()) {
    target_id = OeisSequence(test_id).id;
  }
  int64_t count = 0;
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id] || (target_id > 0 && id != target_id)) {
      continue;
    }
    if (Test::checkIncEval(settings, id, false)) {
      count++;
    }
  }
  Log::get().info("Passed incremental evaluation check for " +
                  std::to_string(count) + " programs");
}

void Commands::testLogEval() {
  initLog(false);
  Log::get().info("Testing logarithmic evaluator");
  Parser parser;
  OeisManager manager(settings);
  auto& stats = manager.getStats();
  int64_t count = 0;
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id]) {
      continue;
    }
    OeisSequence seq(id);
    std::ifstream in(seq.getProgramPath());
    if (!in) {
      continue;
    }
    auto program = parser.parse(in);
    if (LogarithmicEvaluator::hasLogarithmicComplexity(program)) {
      Log::get().info(seq.id_str() + " has logarithmic complexity");
      count++;
    }
  }
  Log::get().info(std::to_string(count) +
                  " programs have logarithmic complexity");
}

void Commands::testPari(const std::string& test_id) {
  initLog(false);
  Parser parser;
  Settings settings;
  Interpreter interpreter(settings);
  Evaluator evaluator(settings);
  IncrementalEvaluator inceval(interpreter);
  OeisManager manager(settings);
  manager.load();
  auto& stats = manager.getStats();
  int64_t good = 0, bad = 0;
  size_t target_id = 0;
  if (!test_id.empty()) {
    target_id = OeisSequence(test_id).id;
  }
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id] || (target_id > 0 && id != target_id)) {
      continue;
    }
    auto seq = manager.getSequences().at(id);
    Program program;
    try {
      program = parser.parse(seq.getProgramPath());
    } catch (std::exception& e) {
      Log::get().warn(std::string(e.what()));
      continue;
    }

    // generate PARI code
    FormulaGenerator generator;
    Formula formula;
    Sequence expSeq;
    try {
      if (!generator.generate(program, id, formula, true) ||
          !Pari::convertToPari(formula)) {
        continue;
      }
    } catch (const std::exception&) {
      // error during formula generation => check evaluation
      bool hasEvalError;
      try {
        evaluator.eval(program, expSeq, 10);
        hasEvalError = false;
      } catch (const std::exception&) {
        hasEvalError = true;
      }
      if (!hasEvalError) {
        Log::get().error("Expected evaluation error for " + seq.id_str(), true);
      }
    }
    auto pariCode = Pari::toString(formula);
    Log::get().info(seq.id_str() + ": " + pariCode);

    // determine number of terms for testing
    size_t numTerms = seq.existingNumTerms();
    if (inceval.init(program) ||
        pariCode.find("binomial") != std::string::npos) {
      numTerms = std::min<size_t>(numTerms, 10);
    }
    if (ProgramUtil::hasOp(program, Operation::Type::SEQ)) {
      numTerms = std::min<size_t>(numTerms, 3);
    }
    if (numTerms == 0) {
      Log::get().error("No known terms", true);
    }

    // evaluate LODA program
    try {
      evaluator.eval(program, expSeq, numTerms);
    } catch (const std::exception&) {
      Log::get().warn("Cannot evaluate " + seq.id_str());
      continue;
    }
    if (expSeq.empty()) {
      Log::get().error("Evaluation error", true);
    }

    // evaluate PARI program
    auto genSeq = Pari::eval(formula, 0, numTerms - 1);

    // compare results
    if (genSeq != expSeq) {
      Log::get().info("Generated sequence: " + genSeq.to_string());
      Log::get().info("Expected sequence:  " + expSeq.to_string());
      Log::get().error("Unexpected PARI sequence", true);
      bad++;
    } else {
      good++;
    }
  }
  Log::get().info(std::to_string(good) + " passed, " + std::to_string(bad) +
                  " failed PARI check");
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
  initLog(true);
  Benchmark benchmark;
  benchmark.smokeTest();
}

void Commands::findSlow(int64_t num_terms, const std::string& type) {
  initLog(false);
  auto t = Operation::Type::NOP;
  if (!type.empty()) {
    t = Operation::Metadata::get(type).type;
  }
  Benchmark benchmark;
  benchmark.findSlow(num_terms, t);
}

void Commands::lists() {
  initLog(false);
  OeisManager manager(settings);
  manager.load();
  manager.generateLists();
}

void Commands::compare(const std::string& path1, const std::string& path2) {
  initLog(true);
  Parser parser;
  Program p1 = parser.parse(getProgramPathAndSeqId(path1).first);
  Program p2 = parser.parse(getProgramPathAndSeqId(path2).first);
  auto id_str = ProgramUtil::getSequenceIdFromProgram(p1);
  OeisSequence seq(id_str);
  OeisManager manager(settings);
  manager.load();
  auto result = manager.getFinder().isOptimizedBetter(p1, p2, seq);
  if (result.empty()) {
    result = "Worse or Equal";
  }
  std::cout << result << std::endl;
}
