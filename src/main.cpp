#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

#include "api_client.hpp"
#include "commands.hpp"
#include "file.hpp"
#include "setup.hpp"
#include "util.hpp"

// must be after util.hpp
#ifdef _WIN64
#include "win_process.hpp"
#else
#include <sys/wait.h>
#include <unistd.h>
typedef int64_t HANDLE;
#endif

int dispatch(Settings settings, const std::vector<std::string>& args);

HANDLE fork(Settings settings, std::vector<std::string> args) {
#ifdef _WIN64
  std::string cmd = "loda.exe";
  settings.printArgs(args);
  for (auto& a : args) {
    cmd += " " + a;
  }
  return create_win_process(cmd);
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

void boinc(Settings settings, const std::vector<std::string>& args) {
  // get project dir
  auto project_env = std::getenv("PROJECT_DIR");
  if (!project_env) {
    Log::get().error("PROJECT_DIR environment variable not set", true);
  }
  std::string project_dir(project_env);
  ensureTrailingSlash(project_dir);

  // set loda home dir
  auto slots_pos = project_dir.find("slots");
  if (slots_pos == std::string::npos) {
    Log::get().error("Missing slots in project dir: " + project_dir, true);
  }
  const std::string loda_home = project_dir.substr(0, slots_pos) + "projects" +
                                FILE_SEP + "boinc.loda-lang.org_loda";
  Setup::setLodaHome(loda_home);

  // mark setup as loaded
  Setup::getMiningMode();

  // ensure git is installed
  if (!hasGit()) {
    Log::get().error("Git not found. Please install it and try again", true);
  }

  // clone programs repository if necessary
  if (!Setup::existsProgramsHome()) {
    FolderLock lock(project_dir);
    if (!Setup::cloneProgramsHome()) {
      Log::get().error("Cannot clone programs repository", true);
    }
  }

  // configure setup
  Setup::setMiningMode(MINING_MODE_CLIENT);
  Setup::forceCPUHours();
  std::ifstream init_data(project_dir + "init_data.xml");
  std::string line;
  std::string submitted_by = "BOINC";
  while (std::getline(init_data, line)) {
    auto start = line.find("<user_name>");
    auto end = line.find("</user_name>");
    if (start != std::string::npos && end != std::string::npos) {
      line = line.substr(start + 11);
      line = line.substr(0, end - start - 11);
      submitted_by = line;
      Log::get().info("Submitting programs as " + submitted_by);
      break;
    }
  }
  Setup::setSubmittedBy(submitted_by);
  return;

  // pick a random miner profile if not mining in parallel
  if (!settings.parallel_mining || settings.num_miner_instances == 1) {
    settings.miner_profile = std::to_string(Random::get().gen() % 100);
  }

  // start mining!
  Commands commands(settings);
  commands.mine();
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
  } else if (cmd == "profile" || cmd == "prof") {
    commands.profile(args.at(1));
  } else if (cmd == "mine") {
    if (settings.parallel_mining) {
      mineParallel(settings, args);
    } else {
      commands.mine();
    }
  } else if (cmd == "submit") {
    if (args.size() == 2) {
      commands.submit(args.at(1), "");
    } else if (args.size() == 3) {
      commands.submit(args.at(1), args.at(2));
    } else {
      std::cout << "invalid number of arguments" << std::endl;
      return 1;
    }
  }
  // hidden boinc command
  else if (cmd == "boinc") {
    boinc(settings, args);
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
    commands.test();
  } else if (cmd == "test-inc-eval") {
    commands.testIncEval();
  } else if (cmd == "dot") {
    commands.dot(args.at(1));
  } else if (cmd == "generate" || cmd == "gen") {
    commands.generate();
  } else if (cmd == "migrate") {
    commands.migrate();
  } else if (cmd == "maintain") {
    commands.maintain(args.at(1));
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
