#include "setup.hpp"

#include <iostream>

#include "file.hpp"
#include "util.hpp"

std::string Setup::LODA_HOME;
std::string Setup::OEIS_HOME;
std::string Setup::PROGRAMS_HOME;

const std::string& Setup::getLodaHome() {
  if (LODA_HOME.empty()) {
    auto loda_home = std::getenv("LODA_HOME");
    if (loda_home) {
      LODA_HOME = std::string(loda_home);
      ensureTrailingSlash(LODA_HOME);
      checkDir(LODA_HOME);
      return LODA_HOME;
    }
    auto user_home = std::string(std::getenv("HOME"));
    LODA_HOME = user_home + "/loda/";
    ensureTrailingSlash(LODA_HOME);
    checkDir(LODA_HOME);
  }
  return LODA_HOME;
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
  PROGRAMS_HOME += "oeis/";
  checkDir(PROGRAMS_HOME);
}

void Setup::checkDir(const std::string& home) {
  if (!isDir(home)) {
    Log::get().error(
        "Directory not found: " + home + " - please run \"loda setup\"", true);
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

  // migrate old folders
  bool migrated = false;
  auto user_home = std::string(std::getenv("HOME"));
  if (isDir(user_home + "/.loda") && !isDir(user_home + "/loda")) {
    std::cout << "You still have LODA data stored in " << user_home << "/.loda"
              << std::endl;
    std::cout << "The new default LODA home directory is " << user_home
              << "/loda" << std::endl;
    std::cout << "Do you want to move it to the new default location? (Y/n) ";
    std::getline(std::cin, line);
    if (line.empty() || line == "y" || line == "Y") {
      migrated = true;
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

  if (!migrated) {
    std::cout << "Enter the directory where LODA should store its local files."
              << std::endl;
    std::cout << "Press enter for the default location in your home directory."
              << std::endl;
    std::cout << "[" << user_home << "/loda] ";
    std::getline(std::cin, line);
  }
}