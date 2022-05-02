#include "boinc.hpp"

#include <fstream>

#include "file.hpp"
#include "miner.hpp"

Boinc::Boinc(const Settings& settings) : settings(settings) {}

void Boinc::run() {
  // get project dir
  auto project_env = std::getenv("PROJECT_DIR");
  if (!project_env) {
    Log::get().error("PROJECT_DIR environment variable not set", true);
  }
  std::string slot_dir(project_env);
  ensureTrailingSlash(slot_dir);

  // read slot init data
  auto init_data = readXML(slot_dir + "init_data.xml");
  auto project_dir = init_data["project_dir"];
  auto user_name = init_data["user_name"];
  if (project_dir.empty() || user_name.empty()) {
    Log::get().error("Invalid project data: " + slot_dir + "init_data.xml",
                     true);
  }
  ensureTrailingSlash(project_dir);

  // initialize setup
  Setup::setLodaHome(project_dir);
  Setup::getMiningMode();
  Setup::setMiningMode(MINING_MODE_CLIENT);
  Setup::setSubmittedBy(user_name);
  Setup::forceCPUHours();

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

  // pick a random miner profile if not mining in parallel
  if (!settings.parallel_mining || settings.num_miner_instances == 1) {
    settings.miner_profile = std::to_string(Random::get().gen() % 100);
  }

  // start mining!
  Miner miner(settings);
  miner.mine();
}

std::map<std::string, std::string> Boinc::readXML(const std::string& path) {
  std::map<std::string, std::string> result;
  std::ifstream in(path);
  std::string line, key, value;
  while (std::getline(in, line)) {
    auto b = line.find('<');
    if (b == std::string::npos) {
      continue;
    }
    line = line.substr(b + 1);
    b = line.find('>');
    if (b == std::string::npos) {
      continue;
    }
    key = line.substr(0, b);
    line = line.substr(b + 1);
    b = line.find("</");
    if (b == std::string::npos) {
      continue;
    }
    value = line.substr(0, b);
    result[key] = value;
    Log::get().debug("read xml tag: " + key + "=" + value);
  }
  return result;
}
