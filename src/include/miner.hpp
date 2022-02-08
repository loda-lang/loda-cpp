#pragma once

#include <map>
#include <memory>

#include "api_client.hpp"
#include "generator.hpp"
#include "matcher.hpp"
#include "mutator.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "setup.hpp"
#include "util.hpp"

class Miner {
 public:
  class Config {
   public:
    std::string name;
    OverwriteMode overwrite_mode;
    std::vector<Generator::Config> generators;
    std::vector<Matcher::Config> matchers;
  };

  Miner(const Settings &settings);

  void mine();

  void submit(const std::string &id, const std::string &path);

 private:
  void checkRegularTasks();

  void reload(bool load_generators, bool force_overwrite = false);

  void ensureSubmittedBy(Program &program);

  void updateSubmittedBy(Program &program);

  static const std::string ANONYMOUS;
  static const int64_t PROGRAMS_TO_FETCH;
  static const int64_t NUM_MUTATIONS;
  const Settings &settings;
  const MiningMode mining_mode;
  std::string profile_name;
  std::unique_ptr<ApiClient> api_client;
  std::unique_ptr<OeisManager> manager;
  std::unique_ptr<MultiGenerator> multi_generator;
  std::unique_ptr<Mutator> mutator;
  AdaptiveScheduler log_scheduler;
  AdaptiveScheduler metrics_scheduler;
  AdaptiveScheduler cpuhours_scheduler;
  AdaptiveScheduler api_scheduler;
  AdaptiveScheduler reload_scheduler;
  int64_t num_processed;
  int64_t num_new;
  int64_t num_updated;
  int64_t num_removed;
  int64_t current_fetch;
  std::map<std::string, int64_t> num_received_per_profile;
};
