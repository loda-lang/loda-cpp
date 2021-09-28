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
  static const bool IS_RELEASE;
};

class Log {
 public:
  enum Level { DEBUG, INFO, WARN, ERROR, ALERT };

  class AlertDetails {
   public:
    std::string text;
    std::string title;
    std::string title_link;
    std::string color;
  };

  Log();

  static Log &get();

  void debug(const std::string &msg);
  void info(const std::string &msg);
  void warn(const std::string &msg);
  void error(const std::string &msg, bool throw_ = false);
  void alert(const std::string &msg, AlertDetails details = AlertDetails());

  Level level;
  bool silent;
  bool loaded_alerts_config;
  bool slack_alerts;
  bool tweet_alerts;

 private:
  int twitter_client;

  void log(Level level, const std::string &msg);
};

class Settings {
 public:
  size_t num_terms;
  int64_t max_memory;
  int64_t max_cycles;
  size_t max_stack_size;
  bool throw_on_overflow;
  bool use_steps;
  std::string miner;

  // flag and offset for printing evaluation results in b-file format
  bool print_as_b_file;
  int64_t print_as_b_file_offset;

  Settings();

  std::vector<std::string> parseArgs(int argc, char *argv[]);
};

class AdaptiveScheduler {
 public:
  AdaptiveScheduler(int64_t target_seconds);

  bool isTargetReached();

  void reset();

 private:
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  const int64_t target_seconds;
  size_t total_checks;
  size_t next_check;
};

class Random {
 public:
  static Random &get();

  uint64_t seed;
  std::mt19937 gen;

 private:
  Random();
};
