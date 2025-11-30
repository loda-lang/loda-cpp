#include <chrono>
#include <iostream>
#include <thread>

#include "cmd/commands.hpp"
#include "mine/api_client.hpp"
#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

// must be after util.hpp
#include "sys/process.hpp"

int dispatch(Settings settings, const std::vector<std::string>& args);

HANDLE fork(Settings settings, std::vector<std::string> args) {
#ifdef _WIN64
  std::string cmd = "loda.exe";
  settings.printArgs(args);
  for (auto& a : args) {
    cmd += " " + a;
  }
  return createWindowsProcess(cmd);
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

void mineParallel(const Settings& settings,
                  const std::vector<std::string>& args) {
  // get relevant settings
  int64_t num_instances = settings.num_miner_instances;
  if (num_instances == 0) {
    num_instances = Setup::getMaxInstances();
  }
  const bool has_miner_profile = settings.miner_profile.empty();
  const bool restart_miners = (settings.num_mine_hours <= 0);

  // prepare instance settings
  auto instance_settings = settings;
  instance_settings.parallel_mining = false;
  instance_settings.report_cpu_hours = false;

  // runtime objects for parallel mining
  std::vector<HANDLE> children_pids(num_instances, 0);
  AdaptiveScheduler cpuhours_scheduler(3600);  // 1 hour (fixed!!)
  ApiClient api_client;

  Log::get().info("Starting parallel mining using " +
                  std::to_string(num_instances) + " instances");
  const auto start_time = std::chrono::steady_clock::now();

  // run miner processes and monitor them
  bool finished = false;
  while (!finished) {
    // check if miner processes are alive, restart them if should not stop
    finished = true;
    for (size_t i = 0; i < children_pids.size(); i++) {
      const auto pid = children_pids[i];
      if (pid != 0 && isChildProcessAlive(pid)) {
        // still alive
        finished = false;
      } else if (pid == 0 || restart_miners) {
        // restart process
        if (has_miner_profile) {
          instance_settings.miner_profile = std::to_string(i);
        }
        children_pids[i] = fork(instance_settings, args);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        finished = false;
      }
    }

    // wait some time
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // report CPU hours
    if (cpuhours_scheduler.isTargetReached()) {
      cpuhours_scheduler.reset();
      if (Setup::shouldReportCPUHours() && settings.report_cpu_hours) {
        // TODO: extend api to do it in one call
        for (int64_t i = 0; i < num_instances; i++) {
          api_client.postCPUHour();
        }
      }
    }
  }

  // finished log message
  auto cur_time = std::chrono::steady_clock::now();
  int64_t mins =
      std::chrono::duration_cast<std::chrono::minutes>(cur_time - start_time)
          .count();
  Log::get().info("Finished parallel mining after " + std::to_string(mins) +
                  " minutes");
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
    Log::get().error("Option -b not allowed for this command", true);
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
  } else if (cmd == "update") {
    commands.update();
  } else if (cmd == "upgrade") {
    commands.upgrade();
  } else if (cmd == "evaluate" || cmd == "eval") {
    commands.evaluate(args.at(1));
  } else if (cmd == "check") {
    commands.check(args.at(1));
  } else if (cmd == "optimize" || cmd == "opt") {
    commands.optimize(args.at(1));
  } else if (cmd == "minimize" || cmd == "min") {
    commands.minimize(args.at(1));
  } else if (cmd == "export") {
    commands.export_(args.at(1));
  } else if (cmd == "profile" || cmd == "prof") {
    commands.profile(args.at(1));
  } else if (cmd == "fold") {
    commands.fold(args.at(1), args.at(2));
  } else if (cmd == "unfold") {
    commands.unfold(args.at(1));
  } else if (cmd == "mine") {
    if (settings.parallel_mining) {
      mineParallel(settings, args);
    } else {
      commands.mine();
    }
  } else if (cmd == "mutate") {
    commands.mutate(args.at(1));
  } else if (cmd == "submit") {
    if (args.size() == 2) {
      commands.submit(args.at(1), "");
    } else if (args.size() == 3) {
      commands.submit(args.at(1), args.at(2));
    } else {
      std::cout << "invalid number of arguments" << std::endl;
      return 1;
    }
  } else if (cmd == "add-to-list") {
    if (args.size() < 3) {
      std::cerr << "Usage: loda add-to-list <sequence_id> <list_filename>"
                << std::endl;
      return 1;
    }
    commands.addToList(args.at(1), args.at(2));
  }
  // hidden boinc command
  else if (cmd == "boinc") {
    commands.boinc();
  }
#ifdef _WIN64
  // hidden helper command for updates on windows
  else if (cmd == "update-windows-executable") {
    // first replace the old binary by the new one
    std::string cmd = "copy /Y \"" + args.at(1) + "\" \"" + args.at(2) + "\"" +
                      getNullRedirect();
    std::cout << std::endl << std::endl;
    if (system(cmd.c_str()) != 0) {
      std::cout << "Error updating executable. Failed command:" << std::endl
                << cmd << std::endl;
      return 1;
    }
    std::cout << "Update installed. Please run \"loda setup\" again"
              << std::endl
              << "to check and complete its configuration." << std::endl;
  }
#endif
#ifndef LODA_VERSION
  // hidden commands (only in development versions)
  else if (cmd == "test") {
    commands.testAll();
  } else if (cmd == "test-fast") {
    commands.testFast();
  } else if (cmd == "test-slow") {
    commands.testSlow();
  } else if (cmd == "test-inceval") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testEval(id, EVAL_INCREMENTAL);
  } else if (cmd == "test-vireval") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testEval(id, EVAL_VIRTUAL);
  } else if (cmd == "test-analyzer") {
    commands.testAnalyzer();
  } else if (cmd == "test-pari") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testPari(id);
  } else if (cmd == "test-lean") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testLean(id);
  } else if (cmd == "test-formula-parser") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testFormulaParser(id);
  } else if (cmd == "test-range") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.testRange(id);
  } else if (cmd == "generate" || cmd == "gen") {
    commands.generate();
  } else if (cmd == "migrate") {
    commands.migrate();
  } else if (cmd == "maintain") {
    std::string id;
    if (args.size() > 1) {
      id = args.at(1);
    }
    commands.maintain(id);
  } else if (cmd == "iterate") {
    commands.iterate(args.at(1));
  } else if (cmd == "benchmark") {
    commands.benchmark();
  } else if (cmd == "find-slow") {
    std::string type;
    if (args.size() > 1) {
      type = args.at(1);
    }
    commands.findSlow(settings.num_terms, type);
  } else if (cmd == "find-slow-formulas") {
    commands.findSlowFormulas();
  } else if (cmd == "find-embseqs") {
    commands.findEmbseqs();
  } else if (cmd == "extract-virseqs") {
    commands.extractVirseqs();
  } else if (cmd == "find-inceval-programs") {
    if (args.size() < 2) {
      std::cerr
          << "Error: find-inceval-programs requires an error code argument"
          << std::endl;
      std::cerr << "Usage: loda find-inceval-programs <error_code|range>"
                << std::endl;
      std::cerr << "Examples:" << std::endl;
      std::cerr << "  loda find-inceval-programs 1         # Find programs "
                   "with error code 1"
                << std::endl;
      std::cerr << "  loda find-inceval-programs 100-200   # Find programs "
                   "with error codes 100-200"
                << std::endl;
      return 1;
    }
    commands.findIncevalPrograms(args.at(1));
  } else if (cmd == "compare") {
    commands.compare(args.at(1), args.at(2));
  } else if (cmd == "replace") {
    commands.replace(args.at(1), args.at(2));
  } else if (cmd == "export-formulas") {
    std::string output_file;
    if (args.size() > 1) {
      output_file = args.at(1);
    }
    commands.exportFormulas(output_file);
  } else if (cmd == "auto-fold") {
    commands.autoFold();
  } else if (cmd == "add-programs") {
    size_t min_commit_count = 5;
    if (args.size() > 1) {
      min_commit_count = std::stoul(args.at(1));
    }
    commands.commitAddedPrograms(min_commit_count);
  } else if (cmd == "update-programs") {
    commands.commitUpdatedAndDeletedPrograms();
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
