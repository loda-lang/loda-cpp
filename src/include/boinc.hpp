#pragma once

#include "util.hpp"

class Boinc {
 public:
  Boinc(const Settings& settings);

  void run();

 private:
  Settings settings;
};
