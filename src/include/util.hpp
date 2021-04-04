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

  class AlertDetails
  {
  public:
    std::string text;
    std::string title;
    std::string title_link;
    std::string color;
  };

  Log();

  static Log& get();

  void debug( const std::string &msg );
  void info( const std::string &msg );
  void warn( const std::string &msg );
  void error( const std::string &msg, bool throw_ = false );
  void alert( const std::string &msg, AlertDetails details = AlertDetails() );

  Level level;
  bool slack_alerts;
  bool tweet_alerts;

private:

  int twitter_client;

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
  size_t max_memory;
  size_t max_cycles;
  size_t max_stack_size;
  size_t max_physical_memory;
  size_t linear_prefix;
  bool optimize_existing_programs;
  bool search_linear;
  bool throw_on_overflow;
  bool use_steps;
  std::string loda_config;

  // flag and offset for printing evaluation results in b-file format
  bool print_as_b_file;
  int64_t print_as_b_file_offset;

  Settings();

  std::vector<std::string> parseArgs( int argc, char *argv[] );

  bool hasMemory() const;

};

std::string getLodaHome();

void ensureDir( const std::string &path );

int64_t getFileAgeInDays( const std::string &path );

size_t getMemUsage();

class FolderLock
{
public:
  FolderLock( std::string folder );

  ~FolderLock();

  void release();

private:
  std::string lockfile;
  int fd;
};
