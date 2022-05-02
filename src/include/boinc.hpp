#pragma once

#include "util.hpp"

class Boinc {
 public:
  Boinc(const Settings& settings);

  void run();

  std::map<std::string, std::string> readXML(const std::string& path);

 private:
  Settings settings;
};
