#pragma once

#include <string>

class Log
{
public:

  enum class Level
  {
    DEBUG,
    INFO,
    WARN,
    ERROR
  };

  static Log& get();

  void debug( const std::string& msg );
  void info( const std::string& msg );
  void warn( const std::string& msg );
  void error( const std::string& msg, bool throw_ = false );

private:
  void log( Level level, const std::string& msg );

};
