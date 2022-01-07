#include <iostream>

#include "commands.hpp"
#include "util.hpp"

int dispatchCommand(const Settings& settings,
                    const std::vector<std::string>& args) {
  // pre-flight checks
  if (args.empty()) {
    Commands::help();
    return EXIT_SUCCESS;
  }
  std::string cmd = args.front();
  if (settings.use_steps && cmd != "evaluate" && cmd != "eval") {
    Log::get().error("Option -s only allowed in evaluate command", true);
  }
  if (settings.print_as_b_file && cmd != "evaluate" && cmd != "eval" &&
      cmd != "check") {
    Log::get().error("Option -b only allowed in evaluate command", true);
  }
  if (settings.parallel_mining && cmd != "mine") {
    Log::get().error("Option -p only allowed in mine command", true);
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
    commands.mine();
  } else if (cmd == "submit") {
    if (args.size() != 3) {
      std::cout << "invalid number of arguments" << std::endl;
      return 1;
    }
    commands.submit(args.at(1), args.at(2));
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
  return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
  Settings settings;
  auto args = settings.parseArgs(argc, argv);
  dispatchCommand(settings, args);
}
