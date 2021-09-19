#include "setup.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "util.hpp"

std::string Setup::LODA_HOME;
std::string Setup::OEIS_HOME;
std::string Setup::PROGRAMS_HOME;
std::string Setup::MINERS_CONFIG;
std::map<std::string, std::string> Setup::ADVANCED_CONFIG;
bool Setup::LOADED_ADVANCED_CONFIG = false;
bool Setup::PRINTED_MEMORY_WARNING = false;
int64_t Setup::MINING_MODE = -1;
int64_t Setup::MAX_MEMORY = -1;
int64_t Setup::UPDATE_INTERVAL = -1;

std::string Setup::getVersionInfo() {
#ifdef LODA_VERSION
  return "LODA v" + std::string(xstr(LODA_VERSION));
#else
  return "LODA developer version";
#endif
}

std::string Setup::getLodaHomeNoCheck() {
  auto loda_home = std::getenv("LODA_HOME");
  std::string result;
  if (loda_home) {
    result = std::string(loda_home);
  } else {
    auto user_home = std::getenv("HOME");
    if (user_home) {
      result = std::string(user_home) + "/loda/";
    } else {
      // on windows...
      auto homedrive = std::getenv("HOMEDRIVE");
      auto homepath = std::getenv("HOMEPATH");
      if (homedrive && homepath) {
        result = std::string(homedrive) + std::string(homepath) + "\\loda/";
      } else {
        throw std::runtime_error(
            "Error determining LODA home directory. Please set the LODA_HOME "
            "environment variable.");
      }
    }
  }
  ensureTrailingSlash(result);
  return result;
}

const std::string& Setup::getLodaHome() {
  if (LODA_HOME.empty()) {
    setLodaHome(getLodaHomeNoCheck());
  }
  return LODA_HOME;
}

void Setup::setLodaHome(const std::string& home) {
  LODA_HOME = home;
  ensureTrailingSlash(LODA_HOME);
  checkDir(LODA_HOME);
  Log::get().info("Using LODA home directory " + LODA_HOME);
}

const std::string& Setup::getMinersConfig() {
  if (MINERS_CONFIG.empty()) {
    setMinersConfig(getLodaHome() + "miners.default.json");
  }
  return MINERS_CONFIG;
}

void Setup::setMinersConfig(const std::string& loda_config) {
  MINERS_CONFIG = loda_config;
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

std::string Setup::getAdvancedConfig(const std::string& key) {
  if (!LOADED_ADVANCED_CONFIG) {
    loadAdvancedConfig();
    LOADED_ADVANCED_CONFIG = true;
  }
  auto it = ADVANCED_CONFIG.find(key);
  if (it != ADVANCED_CONFIG.end()) {
    return it->second;
  }
  return std::string();
}

bool Setup::getAdvancedConfigFlag(const std::string& key) {
  auto s = getAdvancedConfig(key);
  return (s == "yes" || s == "true");
}

int64_t Setup::getAdvancedConfigInt(const std::string& key,
                                    int64_t default_value) {
  auto s = getAdvancedConfig(key);
  if (!s.empty()) {
    return std::stoll(s);
  }
  return default_value;
}

MiningMode Setup::getMiningMode() {
  if (MINING_MODE == -1) {
    auto mode = getAdvancedConfig("LODA_MINING_MODE");
    if (mode == "local") {
      MINING_MODE = static_cast<int64_t>(MINING_MODE_LOCAL);
    } else if (mode == "server") {
      MINING_MODE = static_cast<int64_t>(MINING_MODE_SERVER);
    } else {
      MINING_MODE = static_cast<int64_t>(MINING_MODE_CLIENT);
    }
  }
  return static_cast<MiningMode>(MINING_MODE);
}

int64_t Setup::getMaxMemory() {
  if (MAX_MEMORY == -1) {
    // 1 GB default
    MAX_MEMORY =
        getAdvancedConfigInt("LODA_MAX_PHYSICAL_MEMORY", 1024) * 1024 * 1024;
  }
  return MAX_MEMORY;
}

int64_t Setup::getUpdateIntervalInDays() {
  if (UPDATE_INTERVAL == -1) {
    // 3 days default
    UPDATE_INTERVAL = getAdvancedConfigInt("LODA_UPDATE_INTERVAL", 3);
  }
  return UPDATE_INTERVAL;
}

bool Setup::hasMemory() {
  const auto max_physical_memory = getMaxMemory();
  const auto usage = getMemUsage();
  if (usage > (size_t)(0.95 * max_physical_memory)) {
    if (usage > (size_t)(1.5 * max_physical_memory)) {
      Log::get().error("Exceeded maximum physical memory limit of " +
                           std::to_string(max_physical_memory / (1024 * 1024)) +
                           "MB",
                       true);
    }
    if (!PRINTED_MEMORY_WARNING) {
      Log::get().warn("Reaching maximum physical memory limit of " +
                      std::to_string(max_physical_memory / (1024 * 1024)) +
                      "MB");
      PRINTED_MEMORY_WARNING = true;
    }
    return false;
  }
  return true;
}

void trimString(std::string& str) {
  std::stringstream trimmer;
  trimmer << str;
  str.clear();
  trimmer >> str;
}

void throwSetupParseError(const std::string& line) {
  Log::get().error("Error parsing line from setup.txt: " + line, true);
}

void Setup::loadAdvancedConfig() {
  std::ifstream in(getLodaHomeNoCheck() + "setup.txt");
  if (in.good()) {
    std::string line;
    while (std::getline(in, line)) {
      if (line.empty() || line[0] == '#') {
        continue;
      }
      auto pos = line.find('=');
      if (pos == std::string::npos) {
        throwSetupParseError(line);
      }
      auto key = line.substr(0, pos);
      auto value = line.substr(pos + 1);
      trimString(key);
      trimString(value);
      std::transform(key.begin(), key.end(), key.begin(), ::toupper);
      if (key.empty() || value.empty()) {
        throwSetupParseError(line);
      }
      ADVANCED_CONFIG[key] = value;
    }
  }
}

void Setup::saveAdvancedConfig() {
  std::ofstream out(getLodaHome() + "setup.txt");
  if (out.bad()) {
    Log::get().error("Error saving configuration to setup.txt", true);
  }
  for (auto it : ADVANCED_CONFIG) {
    out << it.first << "=" << it.second << std::endl;
  }
}

void Setup::runWizard() {
  std::string line;
  std::cout << "===== Welcome to " << getVersionInfo() << "! =====" << std::endl
            << "This command will guide you through its setup." << std::endl
            << std::endl;

  // configure home directory
  auto user_home = std::string(std::getenv("HOME"));
  auto loda_home = user_home + "/loda";
  if (std::getenv("LODA_HOME")) {
    loda_home = std::string(std::getenv("LODA_HOME"));
  }
  std::cout << "Enter the directory where LODA should store its files."
            << std::endl;
  std::cout << "Press return for your default location (see below)."
            << std::endl;
  std::cout << "[" << loda_home << "] ";
  std::getline(std::cin, line);
  std::cout << std::endl;
  if (!line.empty()) {
    loda_home = line;
  }
  ensureTrailingSlash(loda_home);
  ensureDir(loda_home);

  // load setup configuration
  LODA_HOME = loda_home;
  loadAdvancedConfig();

  // migrate old programs directory
  auto programs_home = std::getenv("LODA_PROGRAMS_HOME");
  if (programs_home && isDir(std::string(programs_home)) &&
      std::string(programs_home) != user_home + "/loda/programs") {
    std::cout << "Your current location for LODA programs is "
              << std::string(programs_home) << std::endl;
    std::cout << "The new default location for LODA programs is " << user_home
              << "/loda/programs" << std::endl;
    std::cout << "Do you want to move it to the new default location? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      moveDir(std::string(programs_home), user_home + "/loda/programs");
    }
  }

  // initialize programs directory
  if (!isDir(loda_home + "programs/oeis")) {
    std::cout << "LODA needs to download its programs repository from GitHub."
              << std::endl;
    std::cout << "It contains programs for more than 35,000 integer sequences."
              << std::endl;
    std::cout << "It is needed to evaluate known integer sequence programs and "
              << std::endl;
    std::cout << "for mining new programs using the 'loda mine' command."
              << std::endl;
    std::cout << "The repository requires around 350 MB of disk space."
              << std::endl
              << std::endl;
    std::string git_test = "git --version > /dev/null";
    if (system(git_test.c_str()) != 0) {
      std::cout << std::endl
                << "The setup requires the git tool to download the programs."
                << std::endl;
      std::cout
          << "Please install git and restart the setup: "
             "https://git-scm.com/book/en/v2/Getting-Started-Installing-Git"
          << std::endl;
      return;
    }
    std::string git_url = "https://github.com/loda-lang/loda-programs.git";
    std::cout << "Press return to download the default programs repository:"
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

  // migrate old oeis directory
  if (isDir(user_home + "/.loda/oeis") && !isDir(loda_home + "/oeis")) {
    std::cout << "You still have an OEIS index stored in " << user_home
              << "/.loda/oeis" << std::endl;
    std::cout << "The new location of this folder is " << loda_home << "oeis"
              << std::endl;
    std::cout << "Do you want to move it to the new location? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      moveDir(user_home + "/.loda/oeis", loda_home + "/oeis");
    }
  }

  // check binary
  ensureDir(loda_home + "bin/");
  std::string exe;
#ifdef _WIN64
  exe = ".exe";
#endif
  if (!isFile(loda_home + "bin/loda" + exe)) {
    std::cout << "Please copy the 'loda" << exe << "' executable to "
              << loda_home << "bin" << std::endl;
    std::cout << "and restart the setup." << std::endl;
    return;
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

  // mining mode
  std::cout << "LODA supports the following modes for mining programs:"
            << std::endl
            << std::endl;
  std::cout << "1. Local Mode: mined programs are stored in your local"
            << std::endl
            << "   programs folder only." << std::endl
            << std::endl;
  std::cout << "2. Client Mode (default): mined programs are stored in"
            << std::endl
            << "   your local programs folder and also submitted to the"
            << std::endl
            << "   central API server at https://loda-lang.org." << std::endl
            << std::endl;
  std::cout << "3. Server Mode: process submissions from the central API"
            << std::endl
            << "   server and integrate them into the global programs"
            << std::endl
            << "   repository." << std::endl
            << std::endl;
  auto mode = getMiningMode();
  std::cout << "Choose your mining mode:" << std::endl
            << "[" + std::to_string(mode) + "] ";
  std::getline(std::cin, line);
  if (line == "1") {
    mode = MINING_MODE_LOCAL;
  } else if (line == "2") {
    mode = MINING_MODE_CLIENT;
  } else if (line == "3") {
    mode = MINING_MODE_SERVER;
  } else if (!line.empty()) {
    std::cout << "Invalid choice. Please restart the setup." << std::endl;
    return;
  }
  std::string mode_str;
  switch (mode) {
    case MINING_MODE_LOCAL:
      mode_str = "local";
      break;
    case MINING_MODE_CLIENT:
      mode_str = "client";
      break;
    case MINING_MODE_SERVER:
      mode_str = "server";
      break;
  }
  ADVANCED_CONFIG["LODA_MINING_MODE"] = mode_str;
  std::cout << std::endl;

  // check miners config
  const std::string default_miners_config = loda_home + "miners.default.json";
  if (!isFile(default_miners_config)) {
    // TODO: check version here and get the right version!
    std::string url =
        "https://raw.githubusercontent.com/loda-lang/loda-cpp/main/"
        "miners.default.json";
    std::cout << "The LODA miner requires a configuration file." << std::endl;
    std::cout << "Press return to download the default miner configuration:"
              << std::endl;
    std::cout << "[" + url + "] ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      Http::get(url, default_miners_config);
    }
    std::cout << std::endl;
  }

  // max memory usage
  std::cout << "Enter the maximum memory usage of the miner in MB:"
            << std::endl;
  int64_t max_memory = getMaxMemory() / (1024 * 1024);
  std::cout << "[" << max_memory << "] ";
  std::getline(std::cin, line);
  if (!line.empty()) {
    max_memory = std::stoll(line);
  }
  if (max_memory < 512) {
    std::cout << "Invalid value. Please restart the setup." << std::endl;
    return;
  }
  ADVANCED_CONFIG["LODA_MAX_PHYSICAL_MEMORY"] = std::to_string(max_memory);
  std::cout << std::endl;

  // save configuration
  saveAdvancedConfig();

  // good bye
  std::cout << "===== Setup complete. Thanks for using LODA! =====" << std::endl
            << "To run a Hello World example, try 'loda eval A000045'."
            << std::endl
            << "To start mining, use the 'loda mine' command." << std::endl;
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
