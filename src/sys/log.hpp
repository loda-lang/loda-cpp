#pragma once

#include <string>

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

 private:
  void slack(const std::string &msg, AlertDetails details);

  void log(Level level, const std::string &msg);
};
