#include "setup.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "util.hpp"

std::string Setup::USER_HOME;
std::string Setup::LODA_HOME;
std::string Setup::OEIS_HOME;
std::string Setup::PROGRAMS_HOME;
std::string Setup::MINERS_CONFIG;
std::map<std::string, std::string> Setup::SETUP;
bool Setup::LOADED_SETUP = false;
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

std::string getVersionBranch() {
#ifdef LODA_VERSION
  return "v" + std::string(xstr(LODA_VERSION));
#else
  return "main";
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

std::string Setup::getSetupValue(const std::string& key) {
  if (!LOADED_SETUP) {
    loadSetup();
    LOADED_SETUP = true;
  }
  auto it = SETUP.find(key);
  if (it != SETUP.end()) {
    return it->second;
  }
  return std::string();
}

bool Setup::getSetupFlag(const std::string& key) {
  auto s = getSetupValue(key);
  return (s == "yes" || s == "true");
}

int64_t Setup::getSetupInt(const std::string& key, int64_t default_value) {
  auto s = getSetupValue(key);
  if (!s.empty()) {
    return std::stoll(s);
  }
  return default_value;
}

MiningMode Setup::getMiningMode() {
  if (MINING_MODE == -1) {
    auto mode = getSetupValue("LODA_MINING_MODE");
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
    MAX_MEMORY = getSetupInt("LODA_MAX_PHYSICAL_MEMORY", 1024) * 1024 * 1024;
  }
  return MAX_MEMORY;
}

int64_t Setup::getUpdateIntervalInDays() {
  if (UPDATE_INTERVAL == -1) {
    // 1 day default
    UPDATE_INTERVAL = getSetupInt("LODA_UPDATE_INTERVAL", 1);
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

void Setup::loadSetup() {
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
      SETUP[key] = value;
    }
  }
}

void Setup::saveSetup() {
  std::ofstream out(getLodaHome() + "setup.txt");
  if (out.bad()) {
    Log::get().error("Error saving configuration to setup.txt", true);
  }
  for (auto it : SETUP) {
    out << it.first << "=" << it.second << std::endl;
  }
}

void Setup::runWizard() {
  std::string line;
  std::cout << "===== Welcome to " << getVersionInfo() << "! =====" << std::endl
            << "This command will guide you through its setup." << std::endl
            << std::endl;
  checkLodaHome();
  // TODO: check for updates

  loadSetup();

  if (!checkProgramsHome()) {
    return;
  }
  if (!checkExecutable()) {
    return;
  }

  // environment variables
  if (LODA_HOME != USER_HOME + "/loda/") {
    ensureEnvVar("LODA_HOME", LODA_HOME, "Set LODA home directory", true);
  }
  ensureEnvVar("PATH", "$PATH:" + LODA_HOME + "bin",
               "Add LODA command-line tool to path", false);

  if (!checkMiningMode()) {
    return;
  }
  if (!checkMinersConfig()) {
    return;
  }
  if (!checkMaxMemory()) {
    return;
  }

  saveSetup();

  // good bye
  std::cout << "===== Setup complete. Thanks for using LODA! =====" << std::endl
            << "To run a Hello World example, try 'loda eval A000045'."
            << std::endl
            << "To start mining, use the 'loda mine' command." << std::endl;
}

void Setup::checkLodaHome() {
  std::string line;
  USER_HOME = std::string(std::getenv("HOME"));
  LODA_HOME = USER_HOME + "/loda";
  if (std::getenv("LODA_HOME")) {
    LODA_HOME = std::string(std::getenv("LODA_HOME"));
  }
  std::cout << "Enter the directory where LODA should store its files."
            << std::endl;
  std::cout << "Press return for your default location (see below)."
            << std::endl;
  std::cout << "[" << LODA_HOME << "] ";
  std::getline(std::cin, line);
  std::cout << std::endl;
  if (!line.empty()) {
    LODA_HOME = line;
  }
  ensureTrailingSlash(LODA_HOME);
  ensureDir(LODA_HOME);
}

bool Setup::checkProgramsHome() {
  std::string line;
  if (!isDir(LODA_HOME + "programs/oeis")) {
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
    if (!hasGit()) {
      std::cout << std::endl
                << "The setup requires the git tool to download the programs."
                << std::endl;
      std::cout
          << "Please install git and restart the setup: "
             "https://git-scm.com/book/en/v2/Getting-Started-Installing-Git"
          << std::endl;
      return false;
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
        "git clone " + git_url + " " + LODA_HOME + "programs";
    if (system(git_clone.c_str()) != 0) {
      std::cout << std::endl
                << "Error cloning repository. Aborting setup." << std::endl;
      return false;
    }
    std::cout << std::endl;
  }
  return true;
}

bool Setup::checkExecutable() {
  ensureDir(LODA_HOME + "bin/");
  std::string exe;
#ifdef _WIN64
  exe = ".exe";
#endif
  if (!isFile(LODA_HOME + "bin/loda" + exe)) {
    std::cout << "Please copy the 'loda" << exe << "' executable to "
              << LODA_HOME << "bin" << std::endl;
    std::cout << "and restart the setup." << std::endl;
    return false;
  }
  return true;
}

bool Setup::checkMiningMode() {
  std::string line;
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
    return false;
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
  SETUP["LODA_MINING_MODE"] = mode_str;
  std::cout << std::endl;
  return true;
}

bool Setup::checkMinersConfig() {
  std::string line;
  const std::string default_miners_config = LODA_HOME + "miners.default.json";
  // TODO: also check if outdated
  if (!isFile(default_miners_config)) {
    std::string url = "https://raw.githubusercontent.com/loda-lang/loda-cpp/" +
                      getVersionBranch() + "/miners.default.json";
    std::cout << "The LODA miner requires a configuration file." << std::endl;
    std::cout << "Press return to download the default miner configuration:"
              << std::endl;
    std::cout << "[" + url + "] ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      Http::get(url, default_miners_config);
      // TODO: inject version number
      std::ifstream in(default_miners_config);
      std::stringstream buf;
      std::getline(in, line);
      if (line != "{") {
        std::cout << "Unexpected content in " << default_miners_config
                  << std::endl;
        return false;
      }
      buf << line << std::endl;
    }
    std::cout << std::endl;
  }
  return true;
}

bool Setup::checkMaxMemory() {
  std::string line;
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
    return false;
  }
  SETUP["LODA_MAX_PHYSICAL_MEMORY"] = std::to_string(max_memory);
  std::cout << std::endl;
  return true;
}

void Setup::ensureEnvVar(const std::string& key, const std::string& value,
                         const std::string& comment, bool must_have) {
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
      if (must_have) {
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
        out << std::endl;
        out << "# " << comment << std::endl;
        out << kv << std::endl;
        out.close();
        std::cout << "Done. Please run 'source " << bashrc
                  << "' after this setup." << std::endl;
        std::cout << "Press enter to continue the setup." << std::endl;
        std::getline(std::cin, line);
        std::cout << std::endl;
      }
      return;
    }
  }

  if (must_have) {
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
