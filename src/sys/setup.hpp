#pragma once

#include <map>
#include <string>

enum MiningMode {
  MINING_MODE_LOCAL = 1,
  MINING_MODE_CLIENT = 2,
  MINING_MODE_SERVER = 3
};

std::string convertMiningModeToStr(MiningMode mode);
MiningMode convertStrToMiningMode(const std::string& str);

class Setup {
 public:
  static const std::string LODA_SUBMIT_CPU_HOURS;

  static std::string getLodaHomeNoCheck();

  static const std::string& getLodaHome();

  static void setLodaHome(const std::string& home);

  static MiningMode getMiningMode();

  static void setMiningMode(MiningMode mode);

  static std::string getMinersConfig();

  static const std::string getSubmittedBy();

  static void setSubmittedBy(const std::string& submited_by);

  static void setMinersConfig(const std::string& loda_config);

  static const std::string& getOeisHome();

  static const std::string& getProgramsHome();

  static void setProgramsHome(const std::string& home);

  static bool existsProgramsHome();

  static void cloneProgramsHome(
      const std::string& git_url =
          "https://github.com/loda-lang/loda-programs.git");

  static bool pullProgramsHome(bool fail_on_error = true);

  static std::string getSetupValue(const std::string& key);

  static bool getSetupFlag(const std::string& key, bool default_value);

  static int64_t getSetupInt(const std::string& key, int64_t default_value);

  static int64_t getMaxMemory();

  static int64_t getGitHubUpdateInterval();

  static int64_t getOeisUpdateInterval();

  static int64_t getMaxLocalProgramAgeInDays();

  static int64_t getMaxInstances();

  static std::string getLatestVersion();

  static std::string checkLatestedVersion(bool silent);

  static bool hasMemory();

  static bool shouldReportCPUHours();

  static void forceCPUHours();

  static void runWizard();

  static void performUpgrade(const std::string& new_version, bool silent);

 private:
  static constexpr int64_t UNDEFINED_INT = -2;                  // cannot use -1
  static constexpr int64_t DEFAULT_GITHUB_UPDATE_INTERVAL = 1;  // 1 day default
  static constexpr int64_t DEFAULT_OEIS_UPDATE_INTERVAL =
      30;                                                 // 1 month default
  static constexpr int64_t DEFAULT_MAX_PROGRAM_AGE = 14;  // 2 weeks default
  static constexpr int64_t DEFAULT_MAX_PHYSICAL_MEMORY = 1024;  // 1 GB

  static std::string LODA_HOME;
  static std::string OEIS_HOME;
  static std::string PROGRAMS_HOME;
  static std::string MINERS_CONFIG;
  static std::map<std::string, std::string> SETUP;
  static bool LOADED_SETUP;
  static bool PRINTED_MEMORY_WARNING;
  static int64_t MINING_MODE;
  static int64_t MAX_MEMORY;
  static int64_t GITHUB_UPDATE_INTERVAL;
  static int64_t OEIS_UPDATE_INTERVAL;
  static int64_t MAX_PROGRAM_AGE;
  static int64_t MAX_INSTANCES;

  static void checkDir(const std::string& home);
  static void ensureEnvVar(const std::string& key, const std::string& value,
                           const std::string& comment, bool must_have);
  static void loadSetup();
  static void saveSetup();

  static void checkLodaHome();
  static bool checkProgramsHome();
  static bool checkUpgrade();
  static bool checkEnvVars();
  static bool checkMiningMode();
  static bool checkSubmittedBy();
  static bool checkUsageStats();
  static bool checkMaxMemory();
  static bool checkMaxLocalProgramAge();
  static bool checkMaxInstances();

  static bool updateFile(const std::string& local_file, const std::string& url,
                         const std::string& header, const std::string& marker,
                         bool executable);

  static std::string getExecutable(const std::string& suffix);
};
