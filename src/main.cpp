#include <chrono>
#include <iostream>
#include <thread>

#include "commands.hpp"
#include "file.hpp"
#include "setup.hpp"
#include "util.hpp"

// must be after util.hpp
#ifdef _WIN64
#include <windows.h>
#else
#include <sys/wait.h>
#include <unistd.h>
typedef int64_t HANDLE;
#endif

int dispatch(Settings settings, const std::vector<std::string>& args);

HANDLE fork(Settings settings, std::vector<std::string> args) {
#ifdef _WIN64
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  std::string cmd = "loda.exe";
  settings.printArgs(args);
  for (auto& a : args) {
    cmd += " " + a;
  }
  LPSTR c = const_cast<LPSTR>(cmd.c_str());
  // Start the child process.
  if (!CreateProcess(nullptr, c, nullptr, nullptr, false, 0, nullptr, nullptr,
                     &si, &pi)) {
    Log::get().error(
        "Error in CreateProcess: " + std::to_string(GetLastError()), true);
  }
  return pi.hProcess;
#else
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
#endif
}

bool isChildProcessAlive(HANDLE pid) {
  if (pid == 0) {
    return false;
  }
#ifdef _WIN64
  DWORD exit_code = STILL_ACTIVE;
  GetExitCodeProcess(pid, &exit_code);
  if (exit_code != STILL_ACTIVE) {
    CloseHandle(pid);
    return false;
  }
  return true;
#else
  return (waitpid(pid, nullptr, WNOHANG) == 0);
#endif
}

void mineParallel(Settings settings, const std::vector<std::string>& args) {
  const bool set_miner_profile = settings.miner_profile.empty();
  settings.parallel_mining = false;
  const int64_t num_instances = Setup::getMaxInstances();
  std::vector<HANDLE> children_pids(num_instances, 0);
  Log::get().info("Starting parallel mining using " +
                  std::to_string(num_instances) + " miner instances");
  while (true) {
    for (size_t i = 0; i < children_pids.size(); i++) {
      if (!isChildProcessAlive(children_pids[i])) {
        if (set_miner_profile) {
          settings.miner_profile = std::to_string(i);
        }
        children_pids[i] = fork(settings, args);
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
  } else if (cmd == "test-inc-eval") {
    commands.testIncEval();
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
