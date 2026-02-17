#include "sys/setup.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/jute.h"
#include "sys/log.hpp"
#include "sys/util.hpp"
#include "sys/web_client.hpp"

// must be after util.hpp
#include "sys/process.hpp"

const std::string Setup::LODA_SUBMIT_CPU_HOURS("LODA_SUBMIT_CPU_HOURS");

// TODO: use a singlton of Setup
std::string Setup::LODA_HOME;
std::string Setup::SEQS_HOME;
std::string Setup::PROGRAMS_HOME;
std::string Setup::MINERS_CONFIG;
std::map<std::string, std::string> Setup::SETUP;
bool Setup::LOADED_SETUP = false;
bool Setup::PRINTED_MEMORY_WARNING = false;
int64_t Setup::MINING_MODE = UNDEFINED_INT;
int64_t Setup::MAX_MEMORY = UNDEFINED_INT;
int64_t Setup::GITHUB_UPDATE_INTERVAL = UNDEFINED_INT;
int64_t Setup::OEIS_UPDATE_INTERVAL = UNDEFINED_INT;
int64_t Setup::MAX_PROGRAM_AGE = UNDEFINED_INT;
int64_t Setup::MAX_INSTANCES = UNDEFINED_INT;

int64_t getDefaultMaxInstances() {
  int64_t n = std::thread::hardware_concurrency();
  return std::max<int64_t>(n - 2, 2);
}

std::string convertMiningModeToStr(MiningMode mode) {
  switch (mode) {
    case MINING_MODE_LOCAL:
      return "local";
    case MINING_MODE_CLIENT:
      return "client";
    case MINING_MODE_SERVER:
      return "server";
  }
  return "";
}

MiningMode convertStrToMiningMode(const std::string& str) {
  if (str == "local") {
    return MINING_MODE_LOCAL;
  } else if (str == "server") {
    return MINING_MODE_SERVER;
  }
  return MINING_MODE_CLIENT;
}

std::string Setup::getLodaHomeNoCheck() {
  if (!LODA_HOME.empty()) {
    return LODA_HOME;
  }
  auto loda_home = std::getenv("LODA_HOME");
  std::string result;
  if (loda_home) {
    result = std::string(loda_home);
  } else {
    result = getHomeDir() + FILE_SEP + "loda" + FILE_SEP;
  }
  ensureTrailingFileSep(result);
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
  ensureTrailingFileSep(LODA_HOME);
  checkDir(LODA_HOME);
  Log::get().info("Using LODA home directory \"" + LODA_HOME + "\"");
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
    if (age_in_days < 0 || age_in_days >= getGitHubUpdateInterval()) {
      const std::string url =
          "https://raw.githubusercontent.com/loda-lang/loda-cpp/main/"
          "miners.default.json";
      std::remove(default_config.c_str());
      if (!WebClient::get(url, default_config, false, false, false)) {
        // insecure fall-back option
        WebClient::get(url, default_config, false, true, true);
      }
    }
  }
  return default_config;
}

const std::string Setup::getSubmitter() {
  return getSetupValue("LODA_SUBMITTED_BY");
}

void Setup::setSubmitter(const std::string& submitter) {
  getSubmitter();
  SETUP["LODA_SUBMITTED_BY"] = submitter;
}

void Setup::setMinersConfig(const std::string& loda_config) {
  MINERS_CONFIG = loda_config;
}

const std::string& Setup::getSeqsHome() {
  if (SEQS_HOME.empty()) {
    SEQS_HOME = getLodaHome() + "seqs" + FILE_SEP;
    ensureTrailingFileSep(SEQS_HOME);
    ensureDir(SEQS_HOME);
  }
  return SEQS_HOME;
}

const std::string& Setup::getCacheHome() {
  static std::string cache_home;
  if (cache_home.empty()) {
    // don't remove the trailing /
    cache_home = getLodaHome() + "cache" + FILE_SEP;
    ensureDir(cache_home);
  }
  return cache_home;
}

const std::string& Setup::getDebugHome() {
  static std::string debug_home;
  if (debug_home.empty()) {
    // don't remove the trailing /
    debug_home = getLodaHome() + "debug" + FILE_SEP;
    ensureDir(debug_home);
  }
  return debug_home;
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
  ensureTrailingFileSep(PROGRAMS_HOME);
  checkDir(PROGRAMS_HOME);
}

bool Setup::existsProgramsHome() {
  // We cannot use getProgramsHome() here because it checks for existence
  return isDir(getLodaHome() + "programs");
}

void Setup::cloneProgramsHome(const std::string& git_url) {
  // We cannot use getProgramsHome() here because it checks for existence
  Git::clone(git_url, getLodaHome() + "programs",
             Setup::NUM_COMMITS_FOR_PROGRAMS);
}

bool Setup::pullProgramsHome(bool fail_on_error) {
  std::string args =  "pull origin main -q --depth=" + std::to_string(Setup::NUM_COMMITS_FOR_PROGRAMS);
  return Git::git(getProgramsHome(), args, fail_on_error);
}

void Setup::checkDir(const std::string& home) {
  if (!isDir(home)) {
    Log::get().error(
        "Directory not found: " + home + " - please run \"loda setup\"", true);
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

bool Setup::getSetupFlag(const std::string& key, bool default_value) {
  auto s = getSetupValue(key);
  if (s.empty()) {
    return default_value;
  }
  return (s == "yes" || s == "true" || s == "1");
}

int64_t Setup::getSetupInt(const std::string& key, int64_t default_value) {
  auto s = getSetupValue(key);
  if (!s.empty()) {
    return std::stoll(s);
  }
  return default_value;
}

MiningMode Setup::getMiningMode() {
  if (MINING_MODE == UNDEFINED_INT) {
    auto mode = getSetupValue("LODA_MINING_MODE");
    if (mode.empty()) {
      MINING_MODE = static_cast<int64_t>(MINING_MODE_CLIENT);
    } else {
      MINING_MODE = static_cast<int64_t>(convertStrToMiningMode(mode));
    }
  }
  return static_cast<MiningMode>(MINING_MODE);
}

void Setup::setMiningMode(MiningMode mode) { MINING_MODE = mode; }

int64_t Setup::getMaxMemory() {
  if (MAX_MEMORY == UNDEFINED_INT) {
    // 1 GB default
    MAX_MEMORY =
        getSetupInt("LODA_MAX_PHYSICAL_MEMORY", DEFAULT_MAX_PHYSICAL_MEMORY) *
        1024 * 1024;
  }
  return MAX_MEMORY;
}

int64_t Setup::getGitHubUpdateInterval() {
  if (GITHUB_UPDATE_INTERVAL == UNDEFINED_INT) {
    GITHUB_UPDATE_INTERVAL = getSetupInt("LODA_GITHUB_UPDATE_INTERVAL",
                                         DEFAULT_GITHUB_UPDATE_INTERVAL);
  }
  return GITHUB_UPDATE_INTERVAL;
}

int64_t Setup::getOeisUpdateInterval() {
  if (OEIS_UPDATE_INTERVAL == UNDEFINED_INT) {
    OEIS_UPDATE_INTERVAL =
        getSetupInt("LODA_OEIS_UPDATE_INTERVAL", DEFAULT_OEIS_UPDATE_INTERVAL);
  }
  return OEIS_UPDATE_INTERVAL;
}

int64_t Setup::getMaxLocalProgramAgeInDays() {
  if (MAX_PROGRAM_AGE == UNDEFINED_INT) {
    MAX_PROGRAM_AGE =
        getSetupInt("LODA_MAX_PROGRAM_AGE", DEFAULT_MAX_PROGRAM_AGE);
  }
  return MAX_PROGRAM_AGE;
}

int64_t Setup::getMaxInstances() {
  if (MAX_INSTANCES == UNDEFINED_INT) {
    MAX_INSTANCES = getSetupInt("LODA_MAX_INSTANCES", getDefaultMaxInstances());
  }
  return MAX_INSTANCES;
}

bool Setup::hasMemory() {
  const auto max_physical_memory = getMaxMemory();
  const auto usage = getMemUsage();
  if (usage > (size_t)(0.95 * max_physical_memory)) {
    if (usage > (size_t)(2.0 * max_physical_memory)) {
      Log::get().error("Exceeded maximum physical memory limit of " +
                           formatBytes(max_physical_memory) +
                           " (usage: " + formatBytes(usage) + ")",
                       true);
    }
    if (!PRINTED_MEMORY_WARNING) {
      Log::get().warn("Reaching maximum physical memory limit of " +
                      formatBytes(max_physical_memory) +
                      " (usage: " + formatBytes(usage) + ")");
      PRINTED_MEMORY_WARNING = true;
    }
    return false;
  }
  return true;
}

bool Setup::shouldReportCPUHours() {
  return (getMiningMode() == MINING_MODE_CLIENT &&
          getSetupFlag(LODA_SUBMIT_CPU_HOURS, false));
}

void Setup::forceCPUHours() {
  getSetupFlag(LODA_SUBMIT_CPU_HOURS, false);
  SETUP[LODA_SUBMIT_CPU_HOURS] = "yes";
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

  if (!checkUpgrade()) {
    return;
  }
  if (!checkEnvVars()) {
    return;
  }
  if (!checkProgramsHome()) {
    return;
  }
  if (!checkMiningMode()) {
    return;
  }
  if (MINING_MODE == MINING_MODE_CLIENT) {
    if (!checkSubmittedBy()) {
      return;
    }
    if (!checkUsageStats()) {
      return;
    }
  }
  std::cout << "Configure advanced settings? (y/N) ";
  std::getline(std::cin, line);
  std::cout << std::endl;
  if (line == "y" || line == "Y" || line == "yes") {
    if (!checkMaxInstances()) {
      return;
    }
    if (!checkMaxMemory()) {
      return;
    }
#ifdef STD_FILESYSTEM
    if (getMiningMode() == MINING_MODE_CLIENT && !checkMaxLocalProgramAge()) {
      return;
    }
#endif
  }

  saveSetup();

  // good bye
  std::cout << "===== Setup complete. Thanks for using LODA! =====" << std::endl
            << std::endl
            << "To run a Hello World example (Fibonacci numbers):" << std::endl
            << "  loda eval A000045" << std::endl
            << "To mine programs for OEIS sequences (single core):" << std::endl
            << "  loda mine" << std::endl
            << "To mine programs for OEIS sequences (multi core):" << std::endl
            << "  loda mine -p" << std::endl;
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
  ensureTrailingFileSep(LODA_HOME);
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
  if (!existsProgramsHome()) {
    std::cout << "LODA needs to download its programs repository from GitHub."
              << std::endl;
    std::cout << "The repository requires around 650 MB of disk space."
              << std::endl;
    std::cout << "Checking whether git is installed:" << std::endl;
    Git::git("", "--version");
    std::cout << std::endl;
    std::string git_url = "https://github.com/loda-lang/loda-programs.git";
    std::cout << "Press return to download the default programs repository:"
              << std::endl;
    std::cout << "[" << git_url << "] ";
    std::getline(std::cin, line);
    if (!line.empty()) {
      git_url = line;
    }
    cloneProgramsHome(git_url);
    std::cout << std::endl;
  }
  return true;
}

std::string Setup::getLatestVersion() {
  const std::string local_release_info(".latest-release.json");
  const std::string release_info_url(
      "https://api.github.com/repos/loda-lang/loda-cpp/releases/latest");
  if (!WebClient::get(release_info_url, local_release_info, false, false)) {
    // Error already logged by WebClient::get()
    return "";  // return empty string on failure
  }
  const std::string content = getFileAsString(local_release_info);
  std::remove(local_release_info.c_str());
  auto json = jute::parser::parse(content);
  return json["tag_name"].as_string();
}

std::string Setup::checkLatestedVersion(bool silent) {
  if (Version::IS_RELEASE) {
    const auto latest_version = getLatestVersion();
    if (latest_version.empty()) {
      return "";  // failed to check for updates
    }
    if (latest_version != Version::BRANCH) {
      if (!silent) {
        Log::get().info("New LODA version available: " + latest_version);
      }
      return latest_version;
    }
    // Already on the latest version
    if (!silent) {
      Log::get().info("Latest version of LODA is already installed");
    }
  }
  return "";
}

std::string Setup::getExecutable(const std::string& suffix) {
  std::string exe;
#ifdef _WIN64
  exe = ".exe";
#endif
  return getLodaHome() + "bin" + FILE_SEP + "loda" + suffix + exe;
}

void Setup::performUpgrade(const std::string& new_version, bool silent) {
  ensureDir(getLodaHome() + "bin" + FILE_SEP);
  const std::string exec_local = getExecutable("");
#ifdef _WIN64
  // Windows: download ZIP file and extract it
  const std::string zip_file = getLodaHome() + "bin" + FILE_SEP + 
                                "loda-" + Version::PLATFORM + ".zip";
  const std::string extract_dir = getLodaHome() + "bin" + FILE_SEP + 
                                   "loda-" + Version::PLATFORM;
  const std::string zip_url =
      "https://github.com/loda-lang/loda-cpp/releases/download/" + new_version +
      "/loda-" + Version::PLATFORM + ".zip";
  // Clean up any previous download/extraction
  std::remove(zip_file.c_str());
  if (isDir(extract_dir)) {
    rmDirRecursive(extract_dir);
  }
  // Download the ZIP file
  WebClient::get(zip_url, zip_file, true, true);
  // Extract using PowerShell's Expand-Archive
  const std::string extract_cmd = 
      "powershell -Command \"Expand-Archive -Path \\\"" + zip_file + 
      "\\\" -DestinationPath \\\"" + getLodaHome() + "bin\\\" -Force\"";
  if (!execCmd(extract_cmd, false)) {
    std::remove(zip_file.c_str());
    Log::get().error("Failed to extract upgrade archive", true);
  }
  // Move the executable and DLL files from the extracted directory
  const std::string exec_tmp = extract_dir + FILE_SEP + "loda.exe";
  // Copy DLL dependencies to the bin directory
  const std::string bin_dir = getLodaHome() + "bin" + FILE_SEP;
  const std::string libcurl_src = extract_dir + FILE_SEP + "libcurl.dll";
  const std::string zlib_src = extract_dir + FILE_SEP + "zlib1.dll";
  const std::string libcurl_dst = bin_dir + "libcurl.dll";
  const std::string zlib_dst = bin_dir + "zlib1.dll";
  if (isFile(libcurl_src)) {
    moveFile(libcurl_src, libcurl_dst);
  }
  if (isFile(zlib_src)) {
    moveFile(zlib_src, zlib_dst);
  }
  // Use the temporary executable to update the main one
  const std::string cmd = "\"" + exec_tmp + "\" update-windows-executable \"" +
                          exec_tmp + "\" \"" + exec_local + "\"";
  // Clean up the ZIP file
  std::remove(zip_file.c_str());
  createWindowsProcess(cmd);
#else
  // Unix: download executable directly
  const std::string exec_tmp = getExecutable("-" + Version::PLATFORM);
  const std::string exec_url =
      "https://github.com/loda-lang/loda-cpp/releases/download/" + new_version +
      "/loda-" + Version::PLATFORM;
  WebClient::get(exec_url, exec_tmp, true, true);
  makeExecutable(exec_tmp);
  moveFile(exec_tmp, exec_local);
  if (!silent) {
    Log::get().info("Installed upgrade to LODA " + new_version);
  }
#endif
}

bool Setup::checkUpgrade() {
  auto latest_version = checkLatestedVersion(true);
  if (latest_version.empty()) {
    return true;
  }
  std::cout << "LODA " << latest_version << " is available!" << std::endl
            << "Do you want to install the update? (Y/n) ";
  std::string line;
  std::getline(std::cin, line);
  if (line.empty() || line == "y" || line == "Y") {
    performUpgrade(latest_version, true);
#ifndef _WIN64
    std::cout << "Update installed. Restarting setup... " << std::endl
              << std::endl;
    std::string new_setup = getExecutable("") + " setup";
    if (system(new_setup.c_str()) != 0) {
      std::cout << "Error running setup of LODA " << latest_version
                << std::endl;
    }
#endif
    // in any case, we must stop the current setup here
    return false;
  } else {
    std::cout << std::endl;
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
  SETUP["LODA_MINING_MODE"] = convertMiningModeToStr(mode);
  MINING_MODE = mode;
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

bool Setup::checkSubmittedBy() {
  std::string submitted_by = getSubmitter();
  if (submitted_by.empty()) {
    submitted_by = "none";
  }
  std::cout << "If you want to mine programs, LODA can automatically add"
            << std::endl
            << "your name as a comment in the mined programs. If you specify"
            << std::endl
            << "your name and run the miner in client mode, you give consent"
            << std::endl
            << "to submit mined programs with your name and to publish them"
            << std::endl
            << "at https://loda-lang.org and the programs repository at"
            << std::endl
            << "https://github.com/loda-lang/loda-programs." << std::endl
            << std::endl
            << "Enter your name, or \"none\" to not include it in programs:"
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

bool Setup::checkUsageStats() {
  std::cout << "Do you agree to send anonymous statistics to support mining "
               "capacity estimations?";
  bool flag = getSetupFlag(LODA_SUBMIT_CPU_HOURS, false);
  if (flag) {
    std::cout << "(Y/n) ";
  } else {
    std::cout << "(y/N) ";
  }
  std::string line;
  std::getline(std::cin, line);
  std::cout << std::endl;
  if (!line.empty()) {
    if (line == "y" || line == "Y" || line == "yes") {
      SETUP[LODA_SUBMIT_CPU_HOURS] = "yes";
    } else {
      SETUP[LODA_SUBMIT_CPU_HOURS] = "no";
    }
  }
  return true;
}

bool Setup::checkMaxMemory() {
  std::string line;
  std::cout << "Enter the maximum memory usage per miner instance in MB."
            << std::endl
            << "The recommended range is " << DEFAULT_MAX_PHYSICAL_MEMORY
            << " - " << (DEFAULT_MAX_PHYSICAL_MEMORY * 2) << " MB."
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

bool Setup::checkMaxLocalProgramAge() {
  std::string line;
  std::cout << "Enter the maximum age of local programs in days (default: " +
                   std::to_string(DEFAULT_MAX_PROGRAM_AGE) + ")."
            << std::endl
            << "Older programs are automatically removed. Use -1 to disable"
            << std::endl
            << "the automatic clean up:" << std::endl;
  int64_t max_age = getMaxLocalProgramAgeInDays();
  std::cout << "[" << max_age << "] ";
  std::getline(std::cin, line);
  if (!line.empty()) {
    max_age = std::stoll(line);
  }
  SETUP["LODA_MAX_PROGRAM_AGE"] = std::to_string(max_age);
  std::cout << std::endl;
  return true;
}

bool Setup::checkMaxInstances() {
  std::string line;
  std::cout << "Enter the maximum number of parallel miner instances."
            << std::endl
            << "Every instance needs 1 CPU and at least 1 GB memory."
            << std::endl;
  int64_t max_instances = getMaxInstances();
  std::cout << "[" << max_instances << "] ";
  std::getline(std::cin, line);
  if (!line.empty()) {
    max_instances = std::stoll(line);
  }
  if (max_instances <= 0) {
    std::cout << "Invalid value. Please restart the setup." << std::endl;
    return false;
  }
  SETUP["LODA_MAX_INSTANCES"] = std::to_string(max_instances);
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
    std::cout << "Do you want the setup to add it to " << bashrc << "? (Y/n) ";
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
