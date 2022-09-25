#pragma once

#include <chrono>
#include <csignal>
#include <map>
#include <random>
#include <string>
#include <vector>

class Version {
 public:
  static const std::string VERSION;
  static const std::string BRANCH;
  static const std::string INFO;
  static const std::string PLATFORM;
  static const size_t VERSION_HASH;
  static const bool IS_RELEASE;
};

class Settings {
 public:
  static constexpr size_t DEFAULT_NUM_TERMS = 10;
  static constexpr int64_t DEFAULT_MAX_MEMORY = 1000;
  static constexpr int64_t DEFAULT_MAX_CYCLES = 15000000;

  size_t num_terms;
  int64_t max_memory;
  int64_t max_cycles;
  bool use_steps;
  bool parallel_mining;
  bool report_cpu_hours;
  int64_t num_miner_instances;
  int64_t num_mine_hours;
  std::string miner_profile;

  // flag and offset for printing evaluation results in b-file format
  bool print_as_b_file;
  int64_t print_as_b_file_offset;

  Settings();

  std::vector<std::string> parseArgs(int argc, char *argv[]);

  void printArgs(std::vector<std::string> &args);
};

class AdaptiveScheduler {
 public:
  AdaptiveScheduler(int64_t target_seconds);

  bool isTargetReached();

  void reset();

 private:
  const std::chrono::time_point<std::chrono::steady_clock> setup_time;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  const int64_t target_milliseconds;
  size_t current_checks;
  size_t total_checks;
  size_t next_check;
};

class ProgressMonitor {
 public:
  ProgressMonitor(int64_t target_seconds, const std::string &progress_file,
                  const std::string &checkpoint_file, uint64_t checkpoint_key);

  int64_t getElapsedSeconds();

  bool isTargetReached();

  double getProgress();

  void writeProgress();

  uint64_t encode(uint32_t value);
  uint32_t decode(uint64_t value);

 private:
  const std::chrono::time_point<std::chrono::steady_clock> start_time;
  const int64_t target_seconds;
  int64_t checkpoint_seconds;
  const std::string progress_file;
  const std::string checkpoint_file;
  const uint64_t checkpoint_key;
};

class Random {
 public:
  static Random &get();

  uint64_t seed;
  std::mt19937 gen;

 private:
  Random();
};

class Signals {
 public:
  static bool HALT;
};

void trimString(std::string &str);