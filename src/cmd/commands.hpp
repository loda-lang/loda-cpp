#pragma once

#include <string>

#include "sys/util.hpp"

class Commands {
 public:
  explicit Commands(const Settings& settings) : settings(settings) {}

  static void help();

  // official commands

  void setup();

  void update();

  void evaluate(const std::string& path);

  void check(const std::string& path);

  void optimize(const std::string& path);

  void minimize(const std::string& path);

  void export_(const std::string& path);

  void profile(const std::string& path);

  void unfold(const std::string& path);

  void mine();

  void submit(const std::string& path, const std::string& id);

  void mutate(const std::string& path);

  // hidden commands

  void boinc();

  void test();

  void testAnalyzer(const std::string& id);

  void testLogEval();

  void testPari(const std::string& id);

  void dot(const std::string& path);

  void generate();

  void migrate();

  void maintain(const std::string& id);

  void iterate(const std::string& count);

  void benchmark();

  void findSlow(int64_t num_terms, const std::string& type);

  void lists();

  void compare(const std::string& path1, const std::string& path2);

 private:
  const Settings& settings;

  static void initLog(bool silent);
};
