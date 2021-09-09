#include <stdio.h>
#include <stdlib.h>

#include <sstream>
#include <stack>

#include "commands.hpp"
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
#include "util.hpp"

#ifdef _WIN64
#include <io.h>
#else
#include <unistd.h>
#endif

std::string get_program_path(std::string arg) {
  try {
    OeisSequence s(arg);
    return s.getProgramPath();
  } catch (...) {
    // not an ID string
  }
  return arg;
}

int main(int argc, char *argv[]) {
  Settings settings;
  Parser parser;
  auto args = settings.parseArgs(argc, argv);
  if (!args.empty()) {
    std::string cmd = args.front();
    if (settings.use_steps && cmd != "evaluate" && cmd != "eval") {
      Log::get().error("Option -s only allowed in evaluate command", true);
    }
    if (settings.print_as_b_file && cmd != "evaluate" && cmd != "eval" &&
        cmd != "check" && cmd != "collatz") {
      Log::get().error("Option -b only allowed in evaluate command", true);
    }
    if (cmd == "help") {
      Commands::help();
      return 0;
    }

#ifdef LODA_VERSION
    Log::get().info("LODA v" + std::string(xstr(LODA_VERSION)));
#endif

    if (cmd == "test") {
      Commands::test();
    } else if (cmd == "evaluate" || cmd == "eval") {
      Program program = parser.parse(get_program_path(args.at(1)));
      Evaluator evaluator(settings);
      Sequence seq;
      evaluator.eval(program, seq);
      if (!settings.print_as_b_file) {
        std::cout << seq << std::endl;
      }
    } else if (cmd == "check") {
      OeisSequence seq(args.at(1));
      Program program = parser.parse(seq.getProgramPath());
      Evaluator evaluator(settings);
      auto terms = seq.getTerms(100000);  // magic number
      auto result = evaluator.check(program, terms,
                                    OeisSequence::DEFAULT_SEQ_LENGTH, seq.id);
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
    } else if (cmd == "optimize" || cmd == "opt") {
      Program program = parser.parse(get_program_path(args.at(1)));
      Optimizer optimizer(settings);
      optimizer.optimize(program, 2, 1);
      ProgramUtil::print(program, std::cout);
    } else if (cmd == "minimize" || cmd == "min") {
      Program program = parser.parse(get_program_path(args.at(1)));
      Minimizer minimizer(settings);
      minimizer.optimizeAndMinimize(program, 2, 1,
                                    OeisSequence::DEFAULT_SEQ_LENGTH);
      ProgramUtil::print(program, std::cout);
    } else if (cmd == "generate" || cmd == "gen") {
      Log::get().silent = true;
      OeisManager manager(settings);
      MultiGenerator multi_generator(settings, manager.getStats(), false);
      auto program = multi_generator.getGenerator()->generateProgram();
      ProgramUtil::print(program, std::cout);
    } else if (cmd == "dot") {
      Program program = parser.parse(get_program_path(args.at(1)));
      ProgramUtil::exportToDot(program, std::cout);
    } else if (cmd == "mine") {
      Miner miner(settings);
      miner.mine();
    } else if (cmd == "match") {
      Program program = parser.parse(get_program_path(args.at(1)));
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
      Log::get().info("Match result: " + std::to_string(new_) +
                      " new programs, " + std::to_string(updated) + " updated");
    } else if (cmd == "maintain") {
      OeisMaintenance maintenance(settings);
      maintenance.maintain();
    } else if (cmd == "migrate")  // hidden command
    {
      OeisManager manager(settings);
      manager.migrate();
    } else if (cmd == "iterate")  // hidden command
    {
      int64_t count = stoll(args.at(1));
      Iterator it;
      Program p;
      while (count-- > 0) {
        p = it.next();
        //        std::cout << "\x1B[2J\x1B[H";
        ProgramUtil::print(p, std::cout);
        std::cout << std::endl;
        //        std::cin.ignore();
      }
    } else if (cmd == "collatz")  // hidden command
    {
      Program program = parser.parse(std::string(args.at(1)));
      Evaluator evaluator(settings);
      Sequence seq;
      evaluator.eval(program, seq);
      bool is_collatz = Miner::isCollatzValuation(seq);
      std::cout << (is_collatz ? "true" : "false") << std::endl;
    } else {
      std::cerr << "Unknown command: " << cmd << std::endl;
      return EXIT_FAILURE;
    }

  } else {
    Commands::help();
  }
  return EXIT_SUCCESS;
}
