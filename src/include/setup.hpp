#pragma once

#include <map>
#include <string>

enum MiningMode {
  MINING_MODE_LOCAL = 1,
  MINING_MODE_CLIENT = 2,
  MINING_MODE_SERVER = 3
};

class Setup {
 public:
  static std::string getLodaHomeNoCheck();

  static const std::string& getLodaHome();

  static void setLodaHome(const std::string& home);

  static MiningMode getMiningMode();

  static const std::string& getMinersConfig();

  static const std::string getSubmittedBy();

  static void setMinersConfig(const std::string& loda_config);

  static const std::string& getOeisHome();

  static const std::string& getProgramsHome();

  static void setProgramsHome(const std::string& home);

  static std::string getSetupValue(const std::string& key);

  static bool getSetupFlag(const std::string& key);

  static int64_t getSetupInt(const std::string& key, int64_t default_value);

  static int64_t getMaxMemory();

  static int64_t getUpdateIntervalInDays();

  static bool hasMemory();

  static void runWizard();

 private:
  static std::string USER_HOME;
  static std::string LODA_HOME;
  static std::string OEIS_HOME;
  static std::string PROGRAMS_HOME;
  static std::string MINERS_CONFIG;
  static std::map<std::string, std::string> SETUP;
  static bool LOADED_SETUP;
  static bool PRINTED_MEMORY_WARNING;
  static int64_t MINING_MODE;
  static int64_t MAX_MEMORY;
  static int64_t UPDATE_INTERVAL;

  static void checkDir(const std::string& home);
  static void ensureEnvVar(const std::string& key, const std::string& value,
                           const std::string& comment, bool must_have);
  static void loadSetup();
  static void saveSetup();

  static void checkLodaHome();
  static bool checkProgramsHome();
  static bool checkUpdate();
  static bool checkMiningMode();
  static bool checkMinersConfig();
  static bool checkMineParallelScript();
  static bool checkSubmittedBy();
  static bool checkMaxMemory();

  static bool updateFile(const std::string& local_file, const std::string& url,
                         const std::string& header, const std::string& marker,
                         bool executable);
};
