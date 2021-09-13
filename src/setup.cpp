#include "setup.hpp"

#include <fstream>
#include <iostream>

#include "file.hpp"
#include "util.hpp"

std::string Setup::LODA_HOME;
std::string Setup::LODA_CONFIG;
std::string Setup::OEIS_HOME;
std::string Setup::PROGRAMS_HOME;

std::string Setup::getVersionInfo() {
#ifdef LODA_VERSION
  return "LODA v" + std::string(xstr(LODA_VERSION));
#else
  return "LODA developer version";
#endif
}

const std::string& Setup::getLodaHome() {
  if (LODA_HOME.empty()) {
    auto loda_home = std::getenv("LODA_HOME");
    if (loda_home) {
      setLodaHome(std::string(loda_home));
    } else {
      auto user_home = std::string(std::getenv("HOME"));
      setLodaHome(user_home + "/loda/");
    }
  }
  return LODA_HOME;
}

void Setup::setLodaHome(const std::string& home) {
  LODA_HOME = home;
  ensureTrailingSlash(LODA_HOME);
  checkDir(LODA_HOME);
}

const std::string& Setup::getMinersConfig() {
  if (LODA_CONFIG.empty()) {
    setMinersConfig(getLodaHome() + "miners.default.json");
  }
  return LODA_CONFIG;
}

void Setup::setMinersConfig(const std::string& loda_config) {
  LODA_CONFIG = loda_config;
}

const std::string& Setup::getOeisHome() {
  if (OEIS_HOME.empty()) {
    OEIS_HOME = getLodaHome() + "oeis/";
    ensureTrailingSlash(OEIS_HOME);
    ensureDir(OEIS_HOME);
  }
  return OEIS_HOME;
}

const std::string& Setup::getProgramsHome() {
  if (PROGRAMS_HOME.empty()) {
    auto env = std::getenv("LODA_PROGRAMS_HOME");
    if (env) {
      Log::get().warn(
          "LODA_PROGRAMS_HOME environment variable deprecated, please unset "
          "it");
    }
    setProgramsHome(getLodaHome() + "programs/");
  }
  return PROGRAMS_HOME;
}

void Setup::setProgramsHome(const std::string& home) {
  PROGRAMS_HOME = home;
  checkDir(PROGRAMS_HOME);
  ensureTrailingSlash(PROGRAMS_HOME);
  checkDir(PROGRAMS_HOME);
}

void Setup::checkDir(const std::string& home) {
  if (!isDir(home)) {
    Log::get().error(
        "Directory not found: " + home + " - please run 'loda setup'", true);
  }
}

void Setup::ensureTrailingSlash(std::string& dir) {
  if (dir.back() != '/') {
    dir += '/';
  }
}

void Setup::moveDir(const std::string& from, const std::string& to) {
  // Log::get().warn("Moving directory: " + from + " -> " + to);
  std::string cmd = "mv " + from + " " + to;
  if (system(cmd.c_str()) != 0) {
    Log::get().error("Error executing command: " + cmd, true);
  }
}

void Setup::runWizard() {
  std::string line;
  std::cout << "===== Welcome to " << getVersionInfo() << "! =====" << std::endl
            << "This command will guide you through its setup." << std::endl
            << std::endl;

  // migrate old folders
  auto user_home = std::string(std::getenv("HOME"));
  if (isDir(user_home + "/.loda") && !isDir(user_home + "/loda")) {
    std::cout << "You still have LODA data stored in " << user_home << "/.loda"
              << std::endl;
    std::cout << "The new default LODA home directory is " << user_home
              << "/loda" << std::endl;
    std::cout << "Do you want to move it to the new default location? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      moveDir(user_home + "/.loda", user_home + "/loda");
    }
  }
  auto programs_home = std::getenv("LODA_PROGRAMS_HOME");
  if (programs_home && isDir(std::string(programs_home)) &&
      std::string(programs_home) != user_home + "/loda/programs") {
    std::cout << "Your current location for LODA programs is "
              << std::string(programs_home) << std::endl;
    std::cout << "The new default location for LODA programs is " << user_home
              << "/loda" << std::endl;
    std::cout << "Do you want to move it to the new default location? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      moveDir(std::string(programs_home), user_home + "/loda/programs");
    }
  }

  // configure home directory
  auto loda_home = user_home + "/loda";
  if (std::getenv("LODA_HOME")) {
    loda_home = std::string(std::getenv("LODA_HOME"));
  }
  std::cout << "Enter the directory where LODA should store its files."
            << std::endl;
  std::cout << "Press enter for the default location (see below)." << std::endl;
  std::cout << "[" << loda_home << "] ";
  std::getline(std::cin, line);
  std::cout << std::endl;
  if (!line.empty()) {
    loda_home = line;
  }
  ensureTrailingSlash(loda_home);
  ensureDir(loda_home);
  ensureDir(loda_home + "bin/");

  // check binary
  std::string exe;
#ifdef _WIN64
  exe = ".exe";
#endif
  if (!isFile(loda_home + "bin/loda" + exe)) {
    std::cout << "Please copy the 'loda" << exe << "' executable to "
              << loda_home << "bin" << std::endl;
    std::getline(std::cin, line);
  }

  // environment variables
  if (loda_home != user_home + "/loda/") {
    ensureEnvVar("LODA_HOME", loda_home, true);
  }
  ensureEnvVar("PATH", "$PATH:" + loda_home + "bin", false);
  auto env = std::getenv("LODA_PROGRAMS_HOME");
  if (env) {
    std::cout << "The LODA_PROGRAMS_HOME environment variable is deprecated."
              << std::endl
              << "Please remove it from your shell configuration." << std::endl;
    std::getline(std::cin, line);
  }

  // check loda config
  const std::string default_miners_config = loda_home + "miners.default.json";
  if (!isFile(default_miners_config)) {
    // TODO: check version here and get the right version!
    std::string url =
        "https://raw.githubusercontent.com/loda-lang/loda-cpp/main/"
        "miners.default.json";
    std::cout << "To mine programs, you should install the default miner "
                 "configuration file:"
              << std::endl
              << url << std::endl
              << "Download it now ? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      Http::get(url, default_miners_config);
    }
    std::cout << std::endl;
  }

  // initialize programs directory
  if (!isDir(loda_home + "programs/oeis")) {
    std::cout << "You need to install a local copy of the loda-programs "
                 "repository."
              << std::endl;
    std::cout << "This is required for mining and evaluating integer sequence "
                 "programs."
              << std::endl;
    std::string git_test = "git --version > /dev/null";
    if (system(git_test.c_str()) != 0) {
      std::cout << "The setup requires the git tool to download the programs."
                << std::endl;
      std::cout
          << "Please install it: "
             "https://git-scm.com/book/en/v2/Getting-Started-Installing-Git"
          << std::endl;
      std::getline(std::cin, line);
    }
    std::string git_url = "https://github.com/loda-lang/loda-programs.git";
    std::cout << "Enter the git URL for the loda-programs repository (enter "
                 "for default):"
              << std::endl;
    std::cout << "[" << git_url << "] ";
    std::getline(std::cin, line);
    if (!line.empty()) {
      git_url = line;
    }
    std::string git_clone =
        "git clone " + git_url + " " + loda_home + "programs";
    if (system(git_clone.c_str()) != 0) {
      std::cout << std::endl
                << "Error cloning repository. Aborting setup." << std::endl;
      return;
    }
    std::cout << std::endl;
  }

  // good bye
  std::cout << "Setup complete. Thanks for using LODA!" << std::endl
            << "To start mining, run the 'loda mine' command." << std::endl;
}

void Setup::ensureEnvVar(const std::string& key, const std::string& value,
                         bool must) {
  std::string line;
  auto shell = std::getenv("SHELL");
  if (shell) {
    std::string sh(shell);
    if (sh == "/bin/bash") {
      std::string bashrc = std::string(std::getenv("HOME")) + "/.bashrc";
      if (!isFile(bashrc)) {
        bashrc = std::string(std::getenv("HOME")) + "/.bash_profile";
      }
      std::string kv = "export " + key + "=" + value;
      std::ifstream in(bashrc);
      if (in.good()) {
        std::string line;
        while (std::getline(in, line)) {
          if (line == kv) {
            return;  // found!
          }
        }
      }
      if (must) {
        std::cout
            << "The following line must be added to your shell configuration:"
            << std::endl;
      } else {
        std::cout << "We recommend to add the following line to your shell "
                     "configuration:"
                  << std::endl;
      }
      std::cout << kv << std::endl
                << "Do you want the setup to add it to " << bashrc << "? (Y/n)";
      std::getline(std::cin, line);
      std::cout << std::endl;
      if (line.empty() || line == "y" || line == "Y") {
        std::ofstream out(bashrc, std::ios_base::app);
        out << kv << std::endl;
        std::cout << "Ok, please run 'source " << bashrc
                  << "' after this setup." << std::endl;
      }
      return;
    }
  }

  if (must) {
    std::cout << "Please add the following environment variable to your shell "
                 "configuration:"
              << std::endl;
  } else {
    std::cout << "We recommend to add the following environment to your shell "
                 "configuration:"
              << std::endl;
  }
  std::cout << key << "=" << value << std::endl;
  std::getline(std::cin, line);
  std::cout << std::endl;
}
