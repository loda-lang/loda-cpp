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
    ValidationMode validation_mode;
    std::vector<Generator::Config> generators;
    std::vector<Matcher::Config> matchers;
  };

  static constexpr int64_t DEFAULT_LOG_INTERVAL = 120;  // 2 minutes

  Miner(const Settings &settings, int64_t log_interval = DEFAULT_LOG_INTERVAL,
        ProgressMonitor *progress_monitor = nullptr);

  void mine();

  void submit(const std::string &path, std::string id);

 private:
  void runMineLoop();

  bool checkRegularTasks();

  void reload();

  void ensureSubmittedBy(Program &program);

  void updateSubmittedBy(Program &program);

  void reportCPUHour();

  static const std::string ANONYMOUS;
  static const int64_t PROGRAMS_TO_FETCH;
  static const int64_t NUM_MUTATIONS;
  const Settings &settings;
  const MiningMode mining_mode;
  ValidationMode validation_mode;
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
  ProgressMonitor *progress_monitor;
  int64_t num_processed;
  int64_t num_removed;
  int64_t num_reported_hours;
  int64_t current_fetch;
  std::map<std::string, int64_t> num_new_per_user;
  std::map<std::string, int64_t> num_updated_per_user;
};
