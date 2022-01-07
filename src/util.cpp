#include "util.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

#include "file.hpp"
#include "setup.hpp"

#define xstr(a) ystr(a)
#define ystr(a) #a

#ifdef LODA_VERSION
const std::string Version::VERSION = std::string(xstr(LODA_VERSION));
const std::string Version::BRANCH = "v" + std::string(xstr(LODA_VERSION));
const std::string Version::INFO = "LODA v" + std::string(xstr(LODA_VERSION));
const std::string Version::PLATFORM = std::string(xstr(LODA_PLATFORM));
const bool Version::IS_RELEASE = true;
#else
const std::string Version::VERSION = "dev";
const std::string Version::BRANCH = "main";
const std::string Version::INFO = "LODA developer version";
const std::string Version::PLATFORM = "unknown";
const bool Version::IS_RELEASE = false;
#endif

enum TwitterClient {
  TW_UNKNOWN = 0,
  TW_NONE = 1,
  TW_TWIDGE = 2,
  TW_RAINBOWSTREAM = 3
};

TwitterClient findTwitterClient() {
  std::string cmd = "twidge lsfollowers " + getNullRedirect();
  if (system(cmd.c_str()) == 0) {
    return TW_TWIDGE;
  }
  cmd = "rainbowstream -h " + getNullRedirect();
  if (system(cmd.c_str()) == 0) {
    return TW_RAINBOWSTREAM;
  }
  return TW_NONE;
}

Log::Log()
    : level(Level::INFO),
      silent(false),
      loaded_alerts_config(false),
      slack_alerts(false),
      tweet_alerts(false),
      twitter_client(TW_UNKNOWN) {}

Log &Log::get() {
  static Log log;
  return log;
}

void Log::debug(const std::string &msg) { log(Log::Level::DEBUG, msg); }

void Log::info(const std::string &msg) { log(Log::Level::INFO, msg); }

void Log::warn(const std::string &msg) { log(Log::Level::WARN, msg); }

void Log::error(const std::string &msg, bool throw_) {
  log(Log::Level::ERROR, msg);
  if (throw_) {
    throw std::runtime_error(msg);
  }
}

void Log::alert(const std::string &msg, AlertDetails details) {
  log(Log::Level::ALERT, msg);
  if (!loaded_alerts_config) {
    slack_alerts = Setup::getSetupFlag("LODA_SLACK_ALERTS");
    tweet_alerts = Setup::getSetupFlag("LODA_TWEET_ALERTS");
    loaded_alerts_config = true;
  }
  std::string copy = msg;
  if (slack_alerts || tweet_alerts) {
    std::replace(copy.begin(), copy.end(), '"', ' ');
    std::replace(copy.begin(), copy.end(), '\'', ' ');
    if (copy.length() > 140) {
      copy = copy.substr(0, 137);
      while (!copy.empty()) {
        char ch = copy.at(copy.size() - 1);
        copy = copy.substr(0, copy.length() - 1);
        if (ch == ' ' || ch == '.' || ch == ',') break;
      }
      if (!copy.empty()) {
        copy = copy + "...";
      }
    }
  }
  if (!copy.empty()) {
    if (slack_alerts) {
      slack(copy, details);
    }
    if (tweet_alerts) {
      tweet(copy);
    }
  }
}

void Log::slack(const std::string &msg, AlertDetails details) {
  std::string cmd;
  if (!details.text.empty()) {
    std::replace(details.text.begin(), details.text.end(), '"', ' ');
    std::replace(details.title.begin(), details.title.end(), '"', ' ');
    size_t index = 0;
    while (true) {
      index = details.text.find("$", index);
      if (index == std::string::npos) break;
      details.text.replace(index, 1, "\\$");
      index += 2;
    }
    cmd = "slack chat send --text \"" + details.text + "\" --title \"" +
          details.title + "\" --title-link " + details.title_link +
          " --color " + details.color + " --channel \"#miner\"";
  } else {
    cmd = "slack chat send \"" + msg + "\" \"#miner\"";
  }
  static std::string slack_debug;
  if (slack_debug.empty()) {
    slack_debug = Setup::getLodaHome() + "debug" + FILE_SEP + "slack";
    ensureDir(slack_debug);
  }
#ifdef _WIN64
  cmd += " " + getNullRedirect();
#else
  cmd += " > " + slack_debug + ".out 2> " + slack_debug + ".err";
#endif
  auto exit_code = system(cmd.c_str());
  if (exit_code != 0) {
    Log::get().error("Error sending alert to Slack!", false);
    std::ofstream out(slack_debug + ".cmd");
    out << cmd;
    out.close();
  }
}

void Log::tweet(const std::string &msg) {
  if (twitter_client == TW_UNKNOWN) {
    twitter_client = findTwitterClient();
  }
  std::string cmd;
  switch (twitter_client) {
    case TW_UNKNOWN:
      break;
    case TW_NONE:
      break;
    case TW_TWIDGE:
      cmd = "twidge update \"" + msg + "\" " + getNullRedirect();
      break;
    case TW_RAINBOWSTREAM:
      cmd = "printf \"t " + msg + "\\nexit()\\n\" | rainbowstream " +
            getNullRedirect();
      break;
  }
  if (!cmd.empty()) {
    auto exit_code = system(cmd.c_str());
    if (exit_code != 0) {
      Log::get().error("Error tweeting alert! Failed command: " + cmd, false);
    }
  }
}

void Log::log(Level level, const std::string &msg) {
  if (level < this->level || silent) {
    return;
  }
  time_t rawtime;
  char buffer[80];
  time(&rawtime);
  auto timeinfo = localtime(&rawtime);
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
  std::string lev;
  switch (level) {
    case Log::Level::DEBUG:
      lev = "DEBUG";
      break;
    case Log::Level::INFO:
      lev = "INFO ";
      break;
    case Log::Level::WARN:
      lev = "WARN ";
      break;
    case Log::Level::ERROR:
      lev = "ERROR";
      break;
    case Log::Level::ALERT:
      lev = "ALERT";
      break;
  }
  std::cerr << std::string(buffer) << "|" << lev << "|" << msg << std::endl;
}

Settings::Settings()
    : num_terms(DEFAULT_NUM_TERMS),
      max_memory(DEFAULT_MAX_MEMORY),
      max_cycles(DEFAULT_MAX_CYCLES),
      use_steps(false),
      parallel_mining(false),
      print_as_b_file(false),
      print_as_b_file_offset(0) {}

enum class Option {
  NONE,
  NUM_TERMS,
  MAX_MEMORY,
  MAX_CYCLES,
  B_FILE_OFFSET,
  MINER,
  LOG_LEVEL
};

std::vector<std::string> Settings::parseArgs(int argc, char *argv[]) {
  Option option(Option::NONE);
  std::vector<std::string> unparsed;
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (option == Option::NUM_TERMS || option == Option::MAX_MEMORY ||
        option == Option::MAX_CYCLES || option == Option::B_FILE_OFFSET) {
      std::stringstream s(arg);
      int64_t val;
      s >> val;
      if (option != Option::MAX_CYCLES && option != Option::B_FILE_OFFSET &&
          val < 1) {
        Log::get().error("Invalid value for option: " + std::to_string(val),
                         true);
      }
      switch (option) {
        case Option::NUM_TERMS:
          num_terms = val;
          break;
        case Option::B_FILE_OFFSET:
          print_as_b_file = true;
          print_as_b_file_offset = val;
          break;
        case Option::MAX_MEMORY:
          max_memory = val;
          break;
        case Option::MAX_CYCLES:
          max_cycles = val;
          break;
        case Option::LOG_LEVEL:
        case Option::MINER:
        case Option::NONE:
          break;
      }
      option = Option::NONE;
    } else if (option == Option::MINER) {
      miner_profile = arg;
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
      } else if (opt == "m") {
        option = Option::MAX_MEMORY;
      } else if (opt == "c") {
        option = Option::MAX_CYCLES;
      } else if (opt == "i") {
        option = Option::MINER;
      } else if (opt == "s") {
        use_steps = true;
      } else if (opt == "p") {
        parallel_mining = true;
      } else if (opt == "b") {
        option = Option::B_FILE_OFFSET;
      } else if (opt == "l") {
        option = Option::LOG_LEVEL;
      } else {
        Log::get().error("Unknown option: -" + opt, true);
      }
    } else {
      unparsed.push_back(arg);
    }
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
  if (!miner_profile.empty()) {
    args.push_back("-i");
    args.push_back(miner_profile);
  }
  if (print_as_b_file) {
    args.push_back("-b");
    args.push_back(std::to_string(print_as_b_file_offset));
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
    int64_t milliseconds, speed;
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
    speed = (500 * total_checks) / std::max<int64_t>(milliseconds, 1);
    next_check += std::min<int64_t>(std::max<int64_t>(speed, 1), 1000);
    // Log::get().info("next check " + std::to_string(next_check) + " speed " +
    //                std::to_string(speed) + " target " +
    //                std::to_string(target_milliseconds));
  }
  return false;
}

void AdaptiveScheduler::reset() {
  current_checks = 0;
  next_check = 1;
  start_time = std::chrono::steady_clock::now();
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
