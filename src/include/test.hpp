#pragma once

#include <memory>

#include "matcher.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "util.hpp"

class Test {
 public:
  Test();

  void all();

  void number();

  void randomNumber(size_t tests);

  void semantics();

  void sequence();

  void memory();

  void programUtil();

  void iterator(size_t tests);

  void knownPrograms();

  void incEval();

  static bool checkIncEval(const Settings &settings, size_t id,
                           bool mustSupportIncEval);

  void apiClient();

  void checkpoint();

  void oeisList();

  void oeisSeq();

  void steps();

  void blocks();

  void ackermann();

  void optimizer();

  void minimizer(size_t tests);

  void miner();

  void linearMatcher();

  void deltaMatcher();

  void digitMatcher();

  void stats();

  void config();

  void memUsage();

 private:
  std::vector<std::pair<Program, Program>> loadInOutTests(
      const std::string &prefix);

  void testSeq(size_t id, const Sequence &values);

  void testBinary(const std::string &func, const std::string &file,
                  const std::vector<std::vector<int64_t>> &values);

  void testMatcherSet(Matcher &matcher, const std::vector<size_t> &ids);

  void testMatcherPair(Matcher &matcher, size_t id1, size_t id2);

  OeisManager &getManager();

  Settings settings;
  std::unique_ptr<OeisManager> manager_ptr;
};
