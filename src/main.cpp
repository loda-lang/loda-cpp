#include <iostream>

#include "commands.hpp"
#include "util.hpp"

int main(int argc, char *argv[]) {
  // parse command-line arguments
  Settings settings;
  auto args = settings.parseArgs(argc, argv);
  if (args.empty()) {
    Commands::help();
    return EXIT_SUCCESS;
  }
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
    return EXIT_SUCCESS;
  }

  Commands commands(settings);

  // official commands
  if (cmd == "setup") {
    commands.setup();
  } else if (cmd == "evaluate" || cmd == "eval") {
    commands.evaluate(args.at(1));
  } else if (cmd == "check") {
    commands.check(args.at(1));
  } else if (cmd == "optimize" || cmd == "opt") {
    commands.optimize(args.at(1));
  } else if (cmd == "minimize" || cmd == "min") {
    commands.minimize(args.at(1));
  } else if (cmd == "mine") {
    std::vector<std::string> initial_progs;
    for (size_t i = 1; i < args.size(); i++) {
      initial_progs.push_back(args.at(i));
    }
    commands.mine(initial_progs);
  } else if (cmd == "maintain") {
    commands.maintain();
  }
#ifndef LODA_VERSION
  // hidden commands (only in development versions)
  else if (cmd == "test") {
    commands.test();
  } else if (cmd == "dot") {
    commands.dot(args.at(1));
  } else if (cmd == "generate" || cmd == "gen") {
    commands.generate();
  } else if (cmd == "migrate") {
    commands.migrate();
  } else if (cmd == "iterate") {
    commands.iterate(args.at(1));
  } else if (cmd == "benchmark") {
    commands.benchmark();
  }
#endif
  // unknown command
  else {
    std::cerr << "Unknown command: " << cmd << std::endl;
    return EXIT_FAILURE;
  }
}
