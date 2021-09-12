#pragma once

#include <string>

class Setup {
 public:
  static std::string getVersionInfo();

  static const std::string& getLodaHome();

  static void setLodaHome(const std::string& home);

  static const std::string& getLodaConfig();

  static void setLodaConfig(const std::string& loda_config);

  static const std::string& getOeisHome();

  static const std::string& getProgramsHome();

  static void setProgramsHome(const std::string& home);

  static void runWizard();

 private:
  static std::string LODA_HOME;
  static std::string LODA_CONFIG;
  static std::string OEIS_HOME;
  static std::string PROGRAMS_HOME;

  static void checkDir(const std::string& home);
  static void ensureTrailingSlash(std::string& dir);
  static void moveDir(const std::string& from, const std::string& to);
  static void ensureEnvVar(const std::string& key, const std::string& value,
                           bool must);
};
