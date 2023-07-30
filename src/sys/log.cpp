#include "sys/log.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>

#include "sys/file.hpp"
#include "sys/setup.hpp"

Log::Log()
    : level(Level::INFO),
      silent(false),
      loaded_alerts_config(false),
      slack_alerts(false) {}

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
    slack_alerts = Setup::getSetupFlag("LODA_SLACK_ALERTS", false);
    loaded_alerts_config = true;
  }
  std::string copy = msg;
  if (slack_alerts) {
    std::replace(copy.begin(), copy.end(), '"', ' ');
    std::replace(copy.begin(), copy.end(), '\'', ' ');
    if (copy.length() > 140) {
      copy.resize(137);
      while (!copy.empty()) {
        char ch = copy.at(copy.size() - 1);
        copy.pop_back();
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
  }
}

void Log::slack(const std::string &msg, AlertDetails details) {
  std::string cmd;
  if (!details.text.empty()) {
    replaceAll(details.title, "\"", "");
    replaceAll(details.text, "\"", "");
    replaceAll(details.text, "\\/", "\\\\/");
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
