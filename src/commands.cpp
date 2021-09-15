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

void Commands::initLog(bool silent) {
  if (silent) {
    Log::get().silent = true;
  } else {
    Log::get().silent = false;
    Log::get().info("Starting " + Setup::getVersionInfo() +
                    ". See https://loda-lang.org");
  }
}

void Commands::help() {
  initLog(true);
  Settings settings;
  std::cout << "Welcome to " << Setup::getVersionInfo()
            << ". More information at https://loda-lang.org" << std::endl
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
  std::cout << "  setup               Run interactive setup to configure LODA"
            << std::endl;

  std::cout << std::endl << "OEIS Commands:" << std::endl;
  std::cout << "  mine                Mine programs for OEIS sequences (see -i)"
            << std::endl;
  std::cout
      << "  match <asm-file>    Match a program to OEIS sequences (see -i)"
      << std::endl;
  std::cout << "  check <seq-id>      Check a program for an OEIS sequence "
               "(see -b)"
            << std::endl;
  std::cout << "  maintain            Maintain all programs for OEIS sequences"
            << std::endl;
  std::cout << std::endl << "Targets:" << std::endl;
  std::cout
      << "  <asm-file>          Path to a LODA file (file extension: *.asm)"
      << std::endl;
  std::cout << "  <seq-id>            ID of an OEIS integer sequence "
               "(example: A000045)"
            << std::endl;
  std::cout << "  <program>           Either an <asm-file> or a <seq-id>"
            << std::endl;

  std::cout << std::endl << "Options:" << std::endl;
  std::cout << "  -t <number>         Number of sequence terms (default: "
            << settings.num_terms << ")" << std::endl;
  std::cout << "  -b <number>         Print result in b-file format from a "
               "given offset"
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
        if (progs.size() < 1000 || Setup::hasMemory()) {
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
