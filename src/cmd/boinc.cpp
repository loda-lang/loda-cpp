#include "cmd/boinc.hpp"

#include <fstream>
#include <functional>

#include "mine/api_client.hpp"
#include "mine/miner.hpp"
#include "oeis/oeis_list.hpp"
#include "sys/file.hpp"
#include "sys/git.hpp"
#include "sys/log.hpp"

Boinc::Boinc(const Settings& settings) : settings(settings) {}

void Boinc::run() {
  const std::string init_data_xml("init_data.xml");

  // determine slot dir
  std::string slot_dir;
  auto project_env = std::getenv("PROJECT_DIR");
  if (project_env) {
    slot_dir = std::string(project_env);
    ensureTrailingFileSep(slot_dir);
    Log::get().info("Found environment variable: PROJECT_DIR=" + slot_dir);
    if (!isFile(slot_dir + init_data_xml)) {
      slot_dir = "";  // try current working directory instead
    }
  }

  // check init data file
  const std::string init_data_path = slot_dir + init_data_xml;
  Log::get().info("Loading init data from file: " + init_data_path);
  if (!isFile(init_data_path)) {
    Log::get().error("File not found: " + init_data_path, true);
  }

  // read slot init data
  auto init_data = readXML(init_data_path);
  auto project_dir = init_data["project_dir"];
  auto user_name = init_data["user_name"];
  auto wu_name = init_data["wu_name"];
  auto hostid = init_data["hostid"];
  if (project_dir.empty() || user_name.empty() || wu_name.empty()) {
    Log::get().error("Invalid init data", true);
  }
  ensureTrailingFileSep(project_dir);

  // log debugging info
  auto total_mem = getTotalSystemMem() / (1024 * 1024);
  Log::get().info("Platform: " + Version::PLATFORM +
                  ", system memory: " + std::to_string(total_mem) + " MiB");
  Log::get().info("User name: " + user_name + ", host ID: " + hostid);

  // initialize setup
  Setup::setLodaHome(project_dir);
  Setup::getMiningMode();
  Setup::setMiningMode(MINING_MODE_CLIENT);
  Setup::setSubmittedBy(user_name);
  Setup::forceCPUHours();

  // check environment
  Log::get().info("Checking environment");
#ifdef _WIN64
  Git::fixWindowsEnv(project_dir);
  Git::ensureEnv("TMP", project_dir);
  Git::ensureEnv("TEMP", project_dir);
#else
  {
    std::ofstream test_out(getTmpDir() + "test_write.txt");
    if (!test_out) {
      Log::get().warn("Setting tmp dir: " + project_dir);
      setTmpDir(project_dir);
    }
  }
#endif

  // read input data
  auto input_str = getFileAsString(slot_dir + "input");
  if (!input_str.empty() && input_str[0] == '{') {
    auto input = jute::parser::parse(input_str);

    auto minSequenceTerms = input["minSequenceTerms"];
    if (minSequenceTerms.get_type() == jute::JNUMBER) {
      settings.num_terms = minSequenceTerms.as_int();
      Log::get().info("Setting minimum sequence terms to " +
                      std::to_string(settings.num_terms));
    }

    auto maxCycles = input["maxCycles"];
    if (maxCycles.get_type() == jute::JNUMBER) {
      settings.max_cycles = maxCycles.as_int();
      Log::get().info("Setting maximum cycles to " +
                      std::to_string(settings.max_cycles));
    }

    auto minerProfile = input["minerProfile"];
    if (minerProfile.get_type() == jute::JSTRING) {
      settings.miner_profile = minerProfile.as_string();
      Log::get().info("Setting miner profile to \"" + settings.miner_profile +
                      "\"");
    }

    const bool deleteInvalidMatches =
        getJBool(input, "deleteInvalidMatches", false);
    if (deleteInvalidMatches) {
      auto f = OeisList::getListsHome() + OeisList::INVALID_MATCHES_FILE;
      std::remove(f.c_str());
    }
  }

  // pick a random miner profile if not set already
  if ((!settings.parallel_mining || settings.num_miner_instances == 1) &&
      settings.miner_profile.empty()) {
    settings.miner_profile = std::to_string(Random::get().gen() % 100);
  }

  // create initial progress file
  const int64_t target_seconds = settings.num_mine_hours * 3600;
  const std::string progress_file = slot_dir + "fraction_done";
  const std::string checkpoint_file = slot_dir + "checkpoint";
  const uint64_t checkpoint_key = std::hash<std::string>{}(wu_name);
  ProgressMonitor monitor(target_seconds, progress_file, checkpoint_file,
                          checkpoint_key);
  monitor.writeProgress();

  // clone programs repository if necessary
  if (!Setup::existsProgramsHome()) {
    FolderLock lock(project_dir);
    if (!Setup::existsProgramsHome()) {  // need to check again here
      Setup::cloneProgramsHome();
      monitor.writeProgress();
    }
  }

  // start mining!
  Miner miner(settings, &monitor);
  miner.mine();
}
