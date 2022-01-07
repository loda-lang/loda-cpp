#include <unistd.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "commands.hpp"
#include "file.hpp"
#include "setup.hpp"
#include "util.hpp"

int dispatch(Settings settings, const std::vector<std::string>& args);

int64_t fork(Settings settings, const std::vector<std::string>& args) {
  int64_t child_pid = fork();
  if (child_pid == -1) {
    Log::get().error("Error forking process", true);
  } else if (child_pid == 0) {
    // execute child process
    dispatch(settings, args);
  } else {
    // return child's process ID to the parent
    // Log::get().info("Started child process " + std::to_string(child_pid));
    return child_pid;
  }
  return -1;
}

void mineParallel(Settings settings, const std::vector<std::string>& args) {
  const bool set_miner_profile = settings.miner_profile.empty();
  settings.parallel_mining = false;
  int64_t num_instances = Setup::getMaxInstances();
  if (num_instances < 0) {
    num_instances = 2;  // TODO: determine number of cores
  }
  std::vector<int64_t> child_pids(num_instances, 0);
  Log::get().info("Starting parallel mining using " +
                  std::to_string(num_instances) + " miner instances");
  while (true) {
    for (size_t i = 0; i < child_pids.size(); i++) {
      if (!isChildProcessAlive(child_pids[i])) {
        if (set_miner_profile) {
          settings.miner_profile = std::to_string(i);
        }
        child_pids[i] = fork(settings, args);
        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}

int dispatch(Settings settings, const std::vector<std::string>& args) {
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
    if (settings.parallel_mining) {
      mineParallel(settings, args);
    } else {
      commands.mine();
    }
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
  dispatch(settings, args);
}
