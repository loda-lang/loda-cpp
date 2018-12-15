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
    ERROR,
    ALERT
  };

  static Log& get();

  void debug( const std::string& msg );
  void info( const std::string& msg );
  void warn( const std::string& msg );
  void error( const std::string& msg, bool throw_ = false );
  void alert( const std::string& msg );

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
  size_t max_constant;
  size_t max_index;
  std::string operation_types;
  std::string operand_types;
  std::string program_template;

  Settings();

  std::vector<std::string> parseArgs( int argc, char *argv[] );

};
