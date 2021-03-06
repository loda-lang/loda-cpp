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

  void check(const std::string& path);

  void optimize(const std::string& path);

  void minimize(const std::string& path);

  void profile(const std::string& path);

  void mine();

  void submit(const std::string& path, const std::string& id);

  // hidden commands

  void boinc();

  void test();

  void testIncEval();

  void dot(const std::string& path);

  void generate();

  void migrate();

  void maintain(const std::string& id);

  void iterate(const std::string& count);

  void benchmark();

 private:
  const Settings& settings;

  static void initLog(bool silent);
};
