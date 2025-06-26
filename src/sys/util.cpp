#include "sys/util.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "sys/file.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"

#define cstr(a) std::string(xstr(a))
#define xstr(a) ystr(a)
#define ystr(a) #a

#ifdef LODA_VERSION
const std::string Version::VERSION = cstr(LODA_VERSION);
const std::string Version::BRANCH = "v" + cstr(LODA_VERSION);
const std::string Version::INFO = "LODA v" + cstr(LODA_VERSION);
const std::string Version::PLATFORM = cstr(LODA_PLATFORM);
const bool Version::IS_RELEASE = true;
#else
const std::string Version::VERSION = "dev";
const std::string Version::BRANCH = "main";
const std::string Version::INFO = "LODA developer version";
const std::string Version::PLATFORM = "unknown";
const bool Version::IS_RELEASE = false;
#endif

Settings::Settings()
    : num_terms(DEFAULT_NUM_TERMS),
      max_memory(DEFAULT_MAX_MEMORY),
      max_cycles(DEFAULT_MAX_CYCLES),
      max_eval_secs(-1),
      use_steps(false),
      with_deps(false),
      custom_num_terms(false),
      parallel_mining(false),
      report_cpu_hours(true),
      num_miner_instances(0),
      num_mine_hours(0),
      print_as_b_file(false) {}

enum class Option {
  NONE,
  NUM_TERMS,
  MAX_MEMORY,
  MAX_CYCLES,
  MAX_EVAL_SECS,
  NUM_INSTANCES,
  NUM_MINE_HOURS,
  MINER_PROFILE,
  EXPORT_FORMAT,
  LOG_LEVEL
};

std::vector<std::string> Settings::parseArgs(int argc, char *argv[]) {
  Option option(Option::NONE);
  std::vector<std::string> unparsed;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (option == Option::NUM_TERMS || option == Option::MAX_MEMORY ||
        option == Option::MAX_CYCLES || option == Option::MAX_EVAL_SECS ||
        option == Option::NUM_INSTANCES || option == Option::NUM_MINE_HOURS) {
      std::stringstream s(arg);
      int64_t val;
      s >> val;
      if (option != Option::MAX_CYCLES && option != Option::MAX_MEMORY &&
          option != Option::MAX_EVAL_SECS && val < 1) {
        Log::get().error("Invalid value for option: " + std::to_string(val),
                         true);
      }
      switch (option) {
        case Option::NUM_TERMS:
          num_terms = val;
          break;
        case Option::MAX_MEMORY:
          max_memory = val;
          break;
        case Option::MAX_CYCLES:
          max_cycles = val;
          break;
        case Option::MAX_EVAL_SECS:
          max_eval_secs = val;
          break;
        case Option::NUM_INSTANCES:
          num_miner_instances = val;
          break;
        case Option::NUM_MINE_HOURS:
          num_mine_hours = val;
          break;
        case Option::LOG_LEVEL:
        case Option::MINER_PROFILE:
        case Option::EXPORT_FORMAT:
        case Option::NONE:
          break;
      }
      option = Option::NONE;
    } else if (option == Option::MINER_PROFILE) {
      miner_profile = arg;
      option = Option::NONE;
    } else if (option == Option::EXPORT_FORMAT) {
      export_format = arg;
      option = Option::NONE;
    } else if (option == Option::LOG_LEVEL) {
      if (arg == "debug") {
        Log::get().level = Log::Level::DEBUG;
      } else if (arg == "info") {
        Log::get().level = Log::Level::INFO;
      } else if (arg == "warn") {
        Log::get().level = Log::Level::WARN;
      } else if (arg == "error") {
        Log::get().level = Log::Level::ERROR;
      } else if (arg == "alert") {
        Log::get().level = Log::Level::ALERT;
      } else {
        Log::get().error("Unknown log level: " + arg);
      }
      option = Option::NONE;
    } else if (arg.at(0) == '-') {
      std::string opt = arg.substr(1);
      if (opt == "t") {
        option = Option::NUM_TERMS;
        custom_num_terms = true;
      } else if (opt == "m") {
        option = Option::MAX_MEMORY;
      } else if (opt == "c") {
        option = Option::MAX_CYCLES;
      } else if (opt == "z") {
        option = Option::MAX_EVAL_SECS;
      } else if (opt == "i") {
        option = Option::MINER_PROFILE;
      } else if (opt == "o") {
        option = Option::EXPORT_FORMAT;
      } else if (opt == "s") {
        use_steps = true;
      } else if (opt == "d") {
        with_deps = true;
      } else if (opt == "p") {
        parallel_mining = true;
      } else if (opt == "P") {
        parallel_mining = true;
        option = Option::NUM_INSTANCES;
      } else if (opt == "H") {
        option = Option::NUM_MINE_HOURS;
      } else if (opt == "b") {
        print_as_b_file = true;
      } else if (opt == "-no-report-cpu-hours") {
        report_cpu_hours = false;
      } else if (opt == "l") {
        option = Option::LOG_LEVEL;
      } else {
        Log::get().error("Unknown option: -" + opt, true);
      }
    } else {
      unparsed.push_back(arg);
    }
  }
  if (option != Option::NONE) {
    Log::get().error("Missing argument", true);
  }
  return unparsed;
}

void Settings::printArgs(std::vector<std::string> &args) {
  if (num_terms != DEFAULT_NUM_TERMS) {
    args.push_back("-t");
    args.push_back(std::to_string(num_terms));
  }
  if (max_memory != DEFAULT_MAX_MEMORY) {
    args.push_back("-m");
    args.push_back(std::to_string(max_memory));
  }
  if (max_cycles != DEFAULT_MAX_CYCLES) {
    args.push_back("-c");
    args.push_back(std::to_string(max_cycles));
  }
  if (use_steps) {
    args.push_back("-s");
  }
  if (parallel_mining) {
    args.push_back("-p");
  }
  if (num_mine_hours > 0) {
    args.push_back("-H");
    args.push_back(std::to_string(num_mine_hours));
  }
  if (!report_cpu_hours) {
    args.push_back("--no-report-cpu-hours");
  }
  if (!miner_profile.empty()) {
    args.push_back("-i");
    args.push_back(miner_profile);
  }
  if (print_as_b_file) {
    args.push_back("-b");
  }
}

AdaptiveScheduler::AdaptiveScheduler(int64_t target_seconds)
    : setup_time(std::chrono::steady_clock::now()),
      target_milliseconds(target_seconds * 1000),
      total_checks(0) {
  reset();
}

bool AdaptiveScheduler::isTargetReached() {
  current_checks++;
  total_checks++;
  if (current_checks >= next_check) {
    int64_t milliseconds, speed, step;
    const auto current_time = std::chrono::steady_clock::now();
    milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                       current_time - start_time)
                       .count();
    if (milliseconds >= target_milliseconds) {
      return true;
    }
    milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                       current_time - setup_time)
                       .count();
    // check every 500ms
    static constexpr int64_t MAX_STEP = 1000;
    speed = (500 * total_checks) / std::max<int64_t>(milliseconds, 1);
    step = std::min<int64_t>(std::max<int64_t>(speed, 1), MAX_STEP);
    if (step == MAX_STEP && next_check < MAX_STEP) {
      next_check = MAX_STEP;
    } else {
      next_check += step;
    }
  }
  return false;
}

void AdaptiveScheduler::reset() {
  current_checks = 0;
  next_check = 1;
  start_time = std::chrono::steady_clock::now();
}

ProgressMonitor::ProgressMonitor(int64_t target_seconds,
                                 const std::string &progress_file,
                                 const std::string &checkpoint_file,
                                 uint64_t checkpoint_key)
    : start_time(std::chrono::steady_clock::now()),
      target_seconds(target_seconds),
      checkpoint_seconds(0),
      progress_file(progress_file),
      checkpoint_file(checkpoint_file),
      checkpoint_key(checkpoint_key) {
  // validate target seconds
  if (target_seconds <= 0) {
    Log::get().error(
        "Invalid target duration: " + std::to_string(target_seconds), true);
  }
  // try to read checkpoint
  if (!checkpoint_file.empty()) {
    std::ifstream in(checkpoint_file);
    if (in) {
      try {
        uint64_t value;
        in >> value;
        checkpoint_seconds = decode(value);
        Log::get().info("Resuming from checkpoint at " +
                        std::to_string(getProgress() * 100.0) + "%");
      } catch (const std::exception &) {
        Log::get().error("Error reading checkpoint: " + checkpoint_file,
                         false);  // continue without checkpoint
      }
    }
  }
}

int64_t ProgressMonitor::getElapsedSeconds() {
  const auto now = std::chrono::steady_clock::now();
  const int64_t current_seconds =
      std::chrono::duration_cast<std::chrono::seconds>(now - start_time)
          .count();
  return checkpoint_seconds + current_seconds;
}

bool ProgressMonitor::isTargetReached() {
  return getElapsedSeconds() >= target_seconds;
}

double ProgressMonitor::getProgress() {
  double progress = static_cast<double>(getElapsedSeconds()) /
                    static_cast<double>(target_seconds);
  if (progress < 0.0) {
    progress = 0.0;
  }
  if (progress > 1.0) {
    progress = 1.0;
  }
  return progress;
}

void ProgressMonitor::writeProgress() {
  if (!progress_file.empty()) {
    std::ofstream progress_out(progress_file);
    progress_out.precision(3);
    progress_out << std::fixed << getProgress() << std::endl;
  }
  if (!checkpoint_file.empty()) {
    std::ofstream checkpoint_out(checkpoint_file);
    checkpoint_out << encode(getElapsedSeconds()) << std::endl;
  }
}

uint64_t checksum(uint64_t v) {
  // uses only 8 bit
  uint64_t checksum = 0;
  while (v) {
    checksum += (v & 1);
    v = v >> 1;
  }
  return checksum;
}

uint64_t ProgressMonitor::encode(uint32_t value) {
  uint64_t tmp =
      (checkpoint_key >> 16) + static_cast<uint64_t>(value);  // add key
  return tmp + (checksum(tmp) << 48);                         // add checksum
}

uint32_t ProgressMonitor::decode(uint64_t value) {
  uint64_t check = value >> 48;  // extract checksum
  value = (value << 16) >> 16;   // remove checksum
  uint64_t result = value - (checkpoint_key >> 16);
  if (check != checksum(value)) {
    throw std::runtime_error("checksum error");
  }
  return result;
}

Random &Random::get() {
  static Random rand;
  return rand;
}

Random::Random() {
  std::random_device dev;
  seed = dev();
  gen.seed(seed);
}

bool Signals::HALT = false;

void trimString(std::string &str) {
  while (!str.empty()) {
    if (str.front() == ' ') {
      str = str.substr(1);
    } else if (str.back() == ' ') {
      str = str.substr(0, str.size() - 1);
    } else {
      break;
    }
  }
}

void lowerString(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}
