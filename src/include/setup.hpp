#pragma once

#include <string>

class Setup {
 public:
  static const std::string& getLodaHome();

  static const std::string& getOeisHome();

  static const std::string& getProgramsHome();

  static void setProgramsHome(const std::string& home);

 private:
  static std::string LODA_HOME;
  static std::string OEIS_HOME;
  static std::string PROGRAMS_HOME;

  static void checkDir(const std::string& home);
  static void ensureTrailingSlash(std::string& dir);
  static void moveDir(const std::string& from, const std::string& to);
};
