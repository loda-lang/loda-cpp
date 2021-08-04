#pragma once

#include <chrono>
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

class Http
{
public:

  static void get( const std::string& url, const std::string& local_path );

};

class Metrics
{
public:

  struct Entry
  {
    std::string field;
    std::map<std::string, std::string> labels;
    double value;
  };

  Metrics();

  static Metrics& get();

  void write( const std::vector<Entry> entries ) const;

  const int64_t publish_interval;

private:
  std::string host;
  std::string auth;
  int64_t tmp_file_id;
  mutable bool notified;

};

class Settings
{
public:
  size_t num_terms;
  int64_t max_memory;
  int64_t max_cycles;
  size_t max_stack_size;
  size_t max_physical_memory;
  int64_t update_interval_in_days;
  bool throw_on_overflow;
  bool use_steps;
  std::string loda_config;
  std::string miner;

  // flag and offset for printing evaluation results in b-file format
  bool print_as_b_file;
  int64_t print_as_b_file_offset;

  Settings();

  std::vector<std::string> parseArgs( int argc, char *argv[] );

  bool hasMemory() const;

private:

  mutable bool printed_memory_warning;

};

const std::string& getLodaHome();

bool isDir( const std::string& path );

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

class AdaptiveScheduler
{
public:

  AdaptiveScheduler( int64_t target_seconds );

  bool isTargetReached();

  void reset();

private:
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  const int64_t target_seconds;
  size_t total_checks;
  size_t next_check;

};
