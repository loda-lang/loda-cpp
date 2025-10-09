#pragma once

#include <string>

#include "eval/evaluator.hpp"
#include "sys/util.hpp"

class Commands {
 public:
  explicit Commands(const Settings& settings) : settings(settings) {}

  static void help();

  // official commands

  void setup();

  void update();

  void upgrade();

  void evaluate(const std::string& path);

  void check(const std::string& path);

  void optimize(const std::string& path);

  void minimize(const std::string& path);

  void export_(const std::string& path);

  void profile(const std::string& path);

  void fold(const std::string& main_path, const std::string& sub_id);

  void unfold(const std::string& path);

  void mine();

  void submit(const std::string& path, const std::string& id);

  void mutate(const std::string& path);

  // hidden commands

  void replace(const std::string& search_path, const std::string& replace_path);

  void autoFold();

  void boinc();

  void testAll();

  void testFast();

  void testSlow();

  void testEval(const std::string& id, eval_mode_t mode);

  void testAnalyzer();

  void testPari(const std::string& id);

  void testRange(const std::string& id);

  void generate();

  void migrate();

  void maintain(const std::string& id);

  void iterate(const std::string& count);

  void benchmark();

  void findSlow(int64_t num_terms, const std::string& type);

  void findEmbseqs();

  void extractVirseqs();

  void findIncevalPrograms(const std::string& error_code);

  void compare(const std::string& path1, const std::string& path2);

  void addToList(const std::string& seq_id, const std::string& list_filename);

  void commitAddedPrograms(size_t min_commit_count = 5);

  void commitUpdatedAndDeletedPrograms();

 private:
  const Settings& settings;

  static void initLog(bool silent);
};
