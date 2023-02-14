#pragma once

#include "sys/util.hpp"

class Boinc {
 public:
  explicit Boinc(const Settings& settings);

  void run();

 private:
  Settings settings;
};
