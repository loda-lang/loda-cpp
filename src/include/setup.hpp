#pragma once

#include <map>
#include <string>

class Setup {
 public:
  static std::string getVersionInfo();

  static std::string getLodaHomeNoCheck();

  static const std::string& getLodaHome();

  static void setLodaHome(const std::string& home);

  static const std::string& getMinersConfig();

  static void setMinersConfig(const std::string& loda_config);

  static const std::string& getOeisHome();

  static const std::string& getProgramsHome();

  static void setProgramsHome(const std::string& home);

  static std::string getAdvancedConfig(const std::string& key);

  static bool getAdvancedConfigFlag(const std::string& key);

  static int64_t getAdvancedConfigInt(const std::string& key,
                                      int64_t default_value);

  static int64_t getMaxMemory();

  static int64_t getUpdateIntervalInDays();

  static bool hasMemory();

  static void runWizard();

 private:
  static std::string LODA_HOME;
  static std::string OEIS_HOME;
  static std::string PROGRAMS_HOME;
  static std::string MINERS_CONFIG;
  static std::map<std::string, std::string> ADVANCED_CONFIG;
  static bool LOADED_ADVANCED_CONFIG;
  static bool PRINTED_MEMORY_WARNING;

  static void checkDir(const std::string& home);
  static void ensureTrailingSlash(std::string& dir);
  static void moveDir(const std::string& from, const std::string& to);
  static void ensureEnvVar(const std::string& key, const std::string& value,
                           bool must);
  static void loadAdvancedConfig();
  static void saveAdvancedConfig();
};
