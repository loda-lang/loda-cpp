#pragma once

#include <csignal>
#include <map>
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

  Log();

  static Log& get();

  void debug( const std::string &msg );
  void info( const std::string &msg );
  void warn( const std::string &msg );
  void error( const std::string &msg, bool throw_ = false );
  void alert( const std::string &msg );

  Level level;
  bool tweet_alerts;

private:
  void log( Level level, const std::string &msg );

};

class Metrics
{
public:

  Metrics();

  static Metrics& get();

  void write( const std::string &field, const std::map<std::string, std::string> &labels, double value ) const;

private:
  std::string host;

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
  bool optimize_existing_programs;
  bool search_linear;
  bool throw_on_overflow;
  std::string operation_types;
  std::string operand_types;
  std::string program_template;

  Settings();

  std::vector<std::string> parseArgs( int argc, char *argv[] );

  std::string getGeneratorArgs() const;

};
