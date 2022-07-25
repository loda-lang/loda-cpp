#include "boinc.hpp"

#include <fstream>
#include <functional>

#include "api_client.hpp"
#include "file.hpp"
#include "log.hpp"
#include "miner.hpp"

Boinc::Boinc(const Settings& settings) : settings(settings) {}

void Boinc::run() {
  // get project dir
  auto project_env = std::getenv("PROJECT_DIR");
  if (!project_env) {
    Log::get().error("PROJECT_DIR environment variable not set", true);
  }
  std::string slot_dir(project_env);
  ensureTrailingFileSep(slot_dir);

  // read slot init data
  auto init_data = readXML(slot_dir + "init_data.xml");
  auto project_dir = init_data["project_dir"];
  auto user_name = init_data["user_name"];
  auto wu_name = init_data["wu_name"];
  auto hostid = init_data["hostid"];
  if (project_dir.empty() || user_name.empty() || wu_name.empty()) {
    Log::get().error("Invalid project data: " + slot_dir + "init_data.xml",
                     true);
  }
  ensureTrailingFileSep(project_dir);

  // log debugging info
  Log::get().info("Platform: " + Version::PLATFORM +
                  ", user name: " + user_name + ", hostid: " + hostid);

  // initialize setup
  Setup::setLodaHome(project_dir);
  Setup::getMiningMode();
  Setup::setMiningMode(MINING_MODE_CLIENT);
  Setup::setSubmittedBy(user_name);
  Setup::forceCPUHours();

  // check environment
  Log::get().info("Checking environment");
#ifdef _WIN64
  fixWindowsEnv();
  ensureEnv("TMP", project_dir);
  ensureEnv("TEMP", project_dir);
#else
  {
    std::ofstream test_out(getTmpDir() + "test_write.txt");
    if (!test_out) {
      Log::get().warn("Setting tmp dir: " + project_dir);
      setTmpDir(project_dir);
    }
  }
#endif

  // pick a random miner profile if not mining in parallel
  if (!settings.parallel_mining || settings.num_miner_instances == 1) {
    settings.miner_profile = std::to_string(Random::get().gen() % 100);
  }

  // create initial progress file
  const int64_t target_seconds = settings.num_mine_hours * 3600;
  const std::string progress_file = slot_dir + "fraction_done";
  const std::string checkpoint_file = slot_dir + "checkpoint";
  const uint64_t checkpoint_key = std::hash<std::string>{}(wu_name);
  ProgressMonitor progress_monitor(target_seconds, progress_file,
                                   checkpoint_file, checkpoint_key);

  // clone programs repository if necessary
  if (!Setup::existsProgramsHome()) {
    FolderLock lock(project_dir);
    if (!Setup::existsProgramsHome()) {  // need to check again here
      Setup::cloneProgramsHome();
    }
  }

  // start mining!
  Miner miner(settings, &progress_monitor);
  miner.mine();
}
