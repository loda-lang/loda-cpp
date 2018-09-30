#pragma once

#include <string>
#include <vector>

class Log
{
public:

  enum Level
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

  Level level = Level::INFO;

private:
  void log( Level level, const std::string& msg );

};

class Settings
{
public:
  size_t num_terms;
  size_t num_operations;
  size_t max_memory;
  size_t max_cycles;

  Settings();

  std::vector<std::string> parseArgs( int argc, char *argv[] );

};
