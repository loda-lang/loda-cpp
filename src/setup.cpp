#include "setup.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "jute.h"
#include "util.hpp"
#include "web_client.hpp"

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

std::string Setup::getLodaHomeNoCheck() {
  auto loda_home = std::getenv("LODA_HOME");
  std::string result;
  if (loda_home) {
    result = std::string(loda_home);
  } else {
    result = getHomeDir() + FILE_SEP + "loda" + FILE_SEP;
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
  Log::get().info("Using LODA home directory '" + LODA_HOME + "'");
}

std::string Setup::getMinersConfig() {
  if (!MINERS_CONFIG.empty()) {
    return MINERS_CONFIG;
  }
  const std::string custom_config = getLodaHome() + "miners.json";
  if (isFile(custom_config)) {
    return custom_config;
  }
  const std::string default_config = getLodaHome() + "miners.default.json";
  {
    FolderLock lock(getLodaHome());
    const auto age_in_days = getFileAgeInDays(default_config);
    if (age_in_days < 0 || age_in_days >= getUpdateIntervalInDays()) {
      const std::string url =
          "https://raw.githubusercontent.com/loda-lang/loda-cpp/main/"
          "miners.default.json";
      std::remove(default_config.c_str());
      WebClient::get(url, default_config);
    }
  }
  return default_config;
}

const std::string Setup::getSubmittedBy() {
  return getSetupValue("LODA_SUBMITTED_BY");
}

void Setup::setMinersConfig(const std::string& loda_config) {
  MINERS_CONFIG = loda_config;
}

const std::string& Setup::getOeisHome() {
  if (OEIS_HOME.empty()) {
    OEIS_HOME = getLodaHome() + "oeis" + FILE_SEP;
    ensureTrailingSlash(OEIS_HOME);
    ensureDir(OEIS_HOME);
  }
  return OEIS_HOME;
}

const std::string& Setup::getProgramsHome() {
  if (PROGRAMS_HOME.empty()) {
    setProgramsHome(getLodaHome() + "programs" + FILE_SEP);
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
    UPDATE_INTERVAL =
        getSetupInt("LODA_UPDATE_INTERVAL", DEFAULT_UPDATE_INTERVAL);
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
  while (!str.empty()) {
    if (str.front() == ' ') {
      str = str.substr(1);
    } else if (str.back() == ' ') {
      str = str.substr(0, str.size() - 1);
    } else {
      break;
    }
  }
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
  std::cout << "===== Welcome to " << Version::INFO << "! =====" << std::endl
            << std::endl
            << "This command will guide you through its setup." << std::endl
            << std::endl;
  checkLodaHome();

  loadSetup();

  if (!checkUpdate()) {
    return;
  }
  if (!checkEnvVars()) {
    return;
  }
  if (!checkProgramsHome()) {
    return;
  }
#ifndef _WIN64
  if (!checkMineParallelScript()) {
    return;
  }
#endif
  if (!checkMiningMode()) {
    return;
  }
  if (!checkSubmittedBy()) {
    return;
  }
  if (!checkMaxMemory()) {
    return;
  }
  if (!checkUpdateInterval()) {
    return;
  }

  saveSetup();

  // good bye
  std::cout << "===== Setup complete. Thanks for using LODA! =====" << std::endl
            << std::endl
            << "To run a Hello World example (Fibonacci numbers):" << std::endl
            << "  loda eval A000045" << std::endl
            << "To mine programs for OEIS sequences (single core):" << std::endl
            << "  loda mine" << std::endl;
#ifndef _WIN64
  std::cout << "To mine programs for OEIS sequences (multi core):" << std::endl
            << "  mine_parallel.sh" << std::endl;
#endif
}

void Setup::checkLodaHome() {
  std::string line;
  LODA_HOME = getHomeDir() + FILE_SEP + "loda";
  std::string kind = "default";
  if (std::getenv("LODA_HOME")) {
    LODA_HOME = std::string(std::getenv("LODA_HOME"));
    kind = "currently set";
  }
  std::cout << "Enter the directory where LODA should store its files."
            << std::endl;
#ifdef _WIN64
  std::cout << "Note that non-default locations require manually adding"
            << std::endl
            << "the LODA_HOME environment variable to your computer."
            << std::endl;
#endif
  std::cout << "Press return for the " << kind << " location (see below)."
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

bool Setup::checkEnvVars() {
  std::string line;
  if (LODA_HOME != getHomeDir() + FILE_SEP + "loda" + FILE_SEP) {
#ifdef _WIN64
    std::cout << "Please manually set the following environment variable:"
              << std::endl
              << std::endl
              << "LODA_HOME=\"" << LODA_HOME << "\"" << std::endl;
    std::getline(std::cin, line);
#else
    ensureEnvVar("LODA_HOME", LODA_HOME, "Set LODA home directory", true);
#endif
  }
#ifdef _WIN64
  std::cout << "To run LODA from anywhere on your computer, please" << std::endl
            << "manually add it to your path variable (optional): " << std::endl
            << std::endl
            << "PATH=%PATH%;\"" << LODA_HOME << "bin\"" << std::endl;
  std::getline(std::cin, line);
#else
  ensureEnvVar("PATH", "$PATH:" + LODA_HOME + "bin",
               "Add LODA command-line tool to path", false);
#endif
  return true;
}

bool Setup::checkProgramsHome() {
  std::string line;
  if (!isDir(LODA_HOME + "programs" + FILE_SEP + "oeis")) {
    std::cout << "LODA needs to download its programs repository from GitHub."
              << std::endl;
    std::cout << "It contains programs for more than 50,000 integer sequences."
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
          << "Please install git and restart the setup:" << std::endl
          << "https://git-scm.com/book/en/v2/Getting-Started-Installing-Git"
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
        "git clone " + git_url + " \"" + LODA_HOME + "programs\"";
    if (system(git_clone.c_str()) != 0) {
      std::cout << std::endl
                << "Error cloning repository. Aborting setup." << std::endl;
      return false;
    }
    std::cout << std::endl;
  }
  return true;
}

std::string Setup::getLatestVersion() {
  const std::string local_release_info(".latest-release.json");
  const std::string release_info_url(
      "https://api.github.com/repos/loda-lang/loda-cpp/releases/latest");
  WebClient::get(release_info_url, local_release_info, true, true);
  const std::string content = getFileAsString(local_release_info);
  std::remove(local_release_info.c_str());
  auto json = jute::parser::parse(content);
  return json["tag_name"].as_string();
}

void Setup::checkLatestedVersion() {
  if (Version::IS_RELEASE && (Random::get().gen() % 5 == 0)) {
    const auto latest_version = Setup::getLatestVersion();
    if (latest_version != Version::BRANCH) {
      Log::get().info("LODA " + latest_version +
                      " is available. Run 'loda setup' to update.");
    }
  }
}

bool Setup::checkUpdate() {
  ensureDir(LODA_HOME + "bin" + FILE_SEP);
  std::string exe;
#ifdef _WIN64
  exe = ".exe";
#endif
  const std::string exec_local = LODA_HOME + "bin" + FILE_SEP + "loda" + exe;
  auto latest_version = getLatestVersion();
  if (Version::IS_RELEASE &&
      (latest_version != Version::BRANCH || !isFile(exec_local))) {
    std::cout << "LODA " << latest_version << " is available!" << std::endl
              << "Do you want to install the update? (Y/n) ";
    std::string line;
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      const std::string exec_tmp =
          LODA_HOME + "bin" + FILE_SEP + "loda-" + Version::PLATFORM + exe;
      const std::string exec_url =
          "https://github.com/loda-lang/loda-cpp/releases/download/" +
          latest_version + "/loda-" + Version::PLATFORM + exe;
      WebClient::get(exec_url, exec_tmp, true, true);
#ifdef _WIN64
      std::cout << "Run the following commands to complete the update:"
                << std::endl
                << std::endl;
      std::cout << "move /Y " << exec_tmp << " " << exec_local << std::endl;
      std::cout << exec_local << " setup" << std::endl;
      return false;
#endif
      makeExecutable(exec_tmp);
      moveFile(exec_tmp, exec_local);
      std::cout << "Update installed. Restarting setup... " << std::endl
                << std::endl;
      std::string new_setup = exec_local + " setup";
      if (system(new_setup.c_str()) != 0) {
        std::cout << "Error running setup of LODA " << latest_version
                  << std::endl;
      }
      // in any case, we must stop the current setup here
      return false;
    } else {
      std::cout << std::endl;
    }
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

bool Setup::updateFile(const std::string& local_file, const std::string& url,
                       const std::string& header, const std::string& marker,
                       bool executable) {
  std::string line1, line2;
  std::string action = "Installing";
  std::ifstream in(local_file);
  if (in.good()) {
    std::getline(in, line1);
    std::getline(in, line2);
    if (line1 == header && line2 == marker) {
      action.clear();
    } else {
      action = "Updating";
    }
  }
  in.close();
  if (!action.empty()) {
    std::cout << action << " " << local_file << std::endl;
    std::remove(local_file.c_str());
    WebClient::get(url, local_file, true, true);
    if (Version::IS_RELEASE) {
      // inject marker
      std::ifstream in2(local_file);
      std::stringstream buf;
      std::getline(in2, line1);
      if (line1 != header) {
        std::cout << "Unexpected content in " << local_file << std::endl;
        return false;
      }
      buf << line1 << std::endl;
      buf << marker << std::endl;
      while (std::getline(in2, line1)) {
        buf << line1 << std::endl;
      }
      in2.close();
      std::ofstream out(local_file);
      out << buf.str();
      out.close();
    }
    if (executable) {
      makeExecutable(local_file);
    }
    std::cout << std::endl;
  }
  return true;
}

bool Setup::checkMineParallelScript() {
  const std::string local_file =
      LODA_HOME + "bin" + FILE_SEP + "mine_parallel.sh";
  const std::string url =
      "https://raw.githubusercontent.com/loda-lang/loda-cpp/" +
      Version::BRANCH + "/mine_parallel.sh";
  const std::string header = "#!/bin/bash";
  const std::string marker = "loda_version=" + Version::VERSION;
  return updateFile(local_file, url, header, marker, true);
}

bool Setup::checkSubmittedBy() {
  std::string submitted_by = getSubmittedBy();
  if (submitted_by.empty()) {
    submitted_by = "none";
  }
  std::cout
      << "If you want to mine programs, LODA can automatically add your name"
      << std::endl
      << "as a comment in the mined programs. If you specify your name and run"
      << std::endl
      << "the miner in client mode, you give consent to submit mined programs"
      << std::endl
      << "with your name and to publish them at https://loda-lang.org and the"
      << std::endl
      << "programs repository at https://github.com/loda-lang/loda-programs."
      << std::endl
      << "If you like, enter your name below, or 'none' to not include it:"
      << std::endl;
  std::cout << "[" << submitted_by << "] ";
  std::getline(std::cin, submitted_by);
  std::cout << std::endl;
  if (!submitted_by.empty()) {
    if (submitted_by == "none") {
      SETUP.erase("LODA_SUBMITTED_BY");
    } else {
      SETUP["LODA_SUBMITTED_BY"] = submitted_by;
    }
  }
  return true;
}

bool Setup::checkMaxMemory() {
  std::string line;
  std::cout
      << "Enter the maximum memory usage of the miner in MB (default 1024):"
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

bool Setup::checkUpdateInterval() {
  std::string line;
  std::cout << "Enter the update interval for the main OEIS files and the"
            << std::endl
            << "programs repository in days (default 1):" << std::endl;
  int64_t update_interval = getUpdateIntervalInDays();
  std::cout << "[" << update_interval << "] ";
  std::getline(std::cin, line);
  if (!line.empty()) {
    update_interval = std::stoll(line);
  }
  if (update_interval <= 0) {
    std::cout << "Invalid value. Please restart the setup." << std::endl;
    return false;
  }
  if (update_interval == DEFAULT_UPDATE_INTERVAL) {
    SETUP.erase("LODA_UPDATE_INTERVAL");
  } else {
    SETUP["LODA_UPDATE_INTERVAL"] = std::to_string(update_interval);
  }
  std::cout << std::endl;
  return true;
}

void Setup::ensureEnvVar(const std::string& key, const std::string& value,
                         const std::string& comment, bool must_have) {
  std::string line;
  std::string bashrc = getBashRc();
  if (!bashrc.empty()) {
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
                << "configuration:" << std::endl;
    }
    std::cout << kv << std::endl;
    std::cout << "Do you want the setup to add it to " << bashrc << "? (Y/n)";
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
