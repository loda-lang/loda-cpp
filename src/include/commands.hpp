#pragma once

#include <string>

#include "util.hpp"

class Commands {
 public:
  Commands(const Settings& settings) : settings(settings) {}

  static void help();

  // official commands

  void setup();

  void evaluate(const std::string& path);

  void check(const std::string& id);

  void optimize(const std::string& path);

  void minimize(const std::string& path);

  void match(const std::string& path);

  void mine();

  void maintain();

  // hidden commands (only in development versions)

  void test();

  void dot(const std::string& path);

  void generate();

  void migrate();

  void iterate(const std::string& count);

  void collatz(const std::string& path);

 private:
  const Settings& settings;

  static std::string getVersionInfo();

  static void initLog(bool silent);
};
