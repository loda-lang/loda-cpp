#pragma once

#include <map>
#include <memory>

#include "eval/optimizer.hpp"
#include "lang/program.hpp"
#include "math/number.hpp"
#include "mine/api_client.hpp"
#include "mine/generator.hpp"
#include "mine/matcher.hpp"
#include "mine/mine_manager.hpp"
#include "mine/mutator.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

class Miner {
 public:
  class Config {
   public:
    std::string name;
    std::string domains;
    OverwriteMode overwrite_mode;
    ValidationMode validation_mode;
    std::vector<Generator::Config> generators;
    std::vector<Matcher::Config> matchers;

    bool usesBackoff() const {
      return std::any_of(matchers.begin(), matchers.end(),
                         [](const auto &m) { return m.backoff; });
    }
  };

  explicit Miner(const Settings &settings,
                 ProgressMonitor *progress_monitor = nullptr);

  void mine();

  void submit(const std::string &path, std::string id_str);

  void setBaseProgram(const Program &p) { base_program = p; }

 private:
  void runMineLoop();

  bool checkRegularTasks();

  void reload();

  void ensureSubmitter(Program &program);

  void updateSubmitter(Program &program);

  void logProgress(bool report_slow);

  void reportCPUHour();

  static const std::string ANONYMOUS;
  static const int64_t PROGRAMS_TO_FETCH;
  static const int64_t MAX_BACKLOG;
  static const int64_t NUM_MUTATIONS;
  const Settings &settings;
  const MiningMode mining_mode;
  ValidationMode validation_mode;
  bool submit_mode;
  std::string profile_name;
  std::unique_ptr<ApiClient> api_client;
  std::unique_ptr<MineManager> manager;
  std::unique_ptr<MultiGenerator> multi_generator;
  std::unique_ptr<Mutator> mutator;
  AdaptiveScheduler log_scheduler;
  AdaptiveScheduler metrics_scheduler;
  AdaptiveScheduler cpuhours_scheduler;
  AdaptiveScheduler api_scheduler;
  AdaptiveScheduler reload_scheduler;
  ProgressMonitor *progress_monitor;
  Program base_program;
  int64_t num_processed;
  int64_t num_removed;
  int64_t num_reported_hours;
  int64_t current_fetch;
  std::map<std::string, int64_t> num_new_per_user;
  std::map<std::string, int64_t> num_updated_per_user;
};
