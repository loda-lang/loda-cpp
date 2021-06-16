#include "util.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if __MACH__
#include <mach/mach.h>
#endif

bool getEnvFlag( const std::string &var )
{
  auto t = std::getenv( var.c_str() );
  if ( t )
  {
    std::string s( t );
    return (s == "yes" || s == "true");
  }
  return false;
}

int64_t getEnvInt( const std::string &var, int64_t default_value )
{
  auto t = std::getenv( var.c_str() );
  if ( t )
  {
    std::string s( t );
    return std::stoll( s );
  }
  return default_value;
}

enum TwitterClient
{
  TW_UNKNOWN = 0,
  TW_NONE = 1,
  TW_TWIDGE = 2,
  TW_RAINBOWSTREAM = 3
};

TwitterClient findTwitterClient()
{
  if ( system( "twidge lsfollowers > /dev/null 2> /dev/null" ) == 0 )
  {
    return TW_TWIDGE;
  }
  if ( system( "rainbowstream -h > /dev/null 2> /dev/null" ) == 0 )
  {
    return TW_RAINBOWSTREAM;
  }
  return TW_NONE;
}

Log::Log()
    : level( Level::INFO ),
      slack_alerts( getEnvFlag( "LODA_SLACK_ALERTS" ) ),
      tweet_alerts( getEnvFlag( "LODA_TWEET_ALERTS" ) ),
      twitter_client( TW_UNKNOWN )
{
}

Log& Log::get()
{
  static Log log;
  return log;
}

void Log::debug( const std::string &msg )
{
  log( Log::Level::DEBUG, msg );
}

void Log::info( const std::string &msg )
{
  log( Log::Level::INFO, msg );
}

void Log::warn( const std::string &msg )
{
  log( Log::Level::WARN, msg );
}

void Log::error( const std::string &msg, bool throw_ )
{
  log( Log::Level::ERROR, msg );
  if ( throw_ )
  {
    throw std::runtime_error( msg );
  }
}

void Log::alert( const std::string &msg, AlertDetails details )
{
  log( Log::Level::ALERT, msg );
  std::string copy = msg;
  std::replace( copy.begin(), copy.end(), '"', ' ' );
  std::replace( copy.begin(), copy.end(), '\'', ' ' );
  if ( copy.length() > 140 )
  {
    copy = copy.substr( 0, 137 );
    while ( !copy.empty() )
    {
      char ch = copy.at( copy.size() - 1 );
      copy = copy.substr( 0, copy.length() - 1 );
      if ( ch == ' ' || ch == '.' || ch == ',' ) break;
    }
    if ( !copy.empty() )
    {
      copy = copy + "...";
    }
  }
  if ( !copy.empty() )
  {
    if ( slack_alerts )
    {
      std::string cmd;
      if ( !details.text.empty() )
      {
        std::replace( details.text.begin(), details.text.end(), '"', ' ' );
        std::replace( details.title.begin(), details.title.end(), '"', ' ' );
        size_t index = 0;
        while ( true )
        {
          index = details.text.find( "$", index );
          if ( index == std::string::npos ) break;
          details.text.replace( index, 1, "\\$" );
          index += 2;
        }
        cmd = "slack chat send --text \"" + details.text + "\" --title \"" + details.title + "\" --title-link "
            + details.title_link + " --color " + details.color + " --channel \"#miner\" > /dev/null";
      }
      else
      {
        cmd = "slack chat send \"" + copy + "\" \"#miner\" > /dev/null";
      }
      auto exit_code = system( cmd.c_str() );
      if ( exit_code != 0 )
      {
        error( "Error sending alert to Slack! Failed command: " + cmd, false );
        slack_alerts = false;
      }
    }
    if ( tweet_alerts )
    {
      if ( twitter_client == TW_UNKNOWN )
      {
        twitter_client = findTwitterClient();
      }
      std::string cmd;
      switch ( twitter_client )
      {
      case TW_UNKNOWN:
        break;
      case TW_NONE:
        break;
      case TW_TWIDGE:
        cmd = "twidge update \"" + copy + "\" > /dev/null";
        break;
      case TW_RAINBOWSTREAM:
        cmd = "printf \"t " + copy + "\\nexit()\\n\" | rainbowstream > /dev/null";
        break;
      }
      if ( !cmd.empty() )
      {
        auto exit_code = system( cmd.c_str() );
        if ( exit_code != 0 )
        {
          error( "Error tweeting alert! Failed command: " + cmd, false );
          twitter_client = TW_NONE;
        }
      }
    }
  }
}

void Log::log( Level level, const std::string &msg )
{
  if ( level < this->level )
  {
    return;
  }
  time_t rawtime;
  char buffer[80];
  time( &rawtime );
  auto timeinfo = localtime( &rawtime );
  strftime( buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo );
  std::string lev;
  switch ( level )
  {
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
  std::cerr << std::string( buffer ) << "|" << lev << "|" << msg << std::endl;
}

Metrics::Metrics()
    : publish_interval( getEnvInt( "LODA_METRICS_PUBLISH_INTERVAL", 60 ) )
{
  auto h = std::getenv( "LODA_INFLUXDB_HOST" );
  if ( h )
  {
    host = std::string( h );
    Log::get().info( "Publishing metrics every " + std::to_string( publish_interval ) + "s to InfluxDB at " + host );
  }
  std::random_device rand;
  tmp_file_id = rand() % 1000;
}

Metrics& Metrics::get()
{
  static Metrics metrics;
  return metrics;
}

void Metrics::write( const std::vector<Entry> entries ) const
{
  if ( Metrics::host.empty() )
  {
    return;
  }
  const std::string file_name = "/tmp/loda_metrics_" + std::to_string( tmp_file_id ) + ".txt";
  std::ofstream out( file_name );
  for ( auto& entry : entries )
  {
    out << entry.field;
    for ( auto &l : entry.labels )
    {
      out << "," << l.first << "=" << l.second;
    }
    out << " value=" << entry.value << "\n";
  }
  out.close();
  const std::string cmd = "curl -i -s -XPOST '" + host + "/write?db=loda' --data-binary @" + file_name + " > /dev/null";
  auto exit_code = system( cmd.c_str() );
  if ( exit_code != 0 )
  {
    Log::get().error( "Error publishing metrics; error code " + std::to_string( exit_code ), false );
  }
  std::remove( file_name.c_str() );
}

Settings::Settings()
    : num_terms( 20 ),
      max_memory( getEnvInt( "LODA_MAX_MEMORY", 100000 ) ),
      max_cycles( getEnvInt( "LODA_MAX_CYCLES", 10000000 ) ),
      max_stack_size( getEnvInt( "LODA_MAX_STACK_SIZE", 100 ) ),
      max_physical_memory( getEnvInt( "LODA_MAX_PHYSICAL_MEMORY", 1024 ) * 1024 * 1024 ),
      linear_prefix( 1 ),
      search_linear( false ),
      throw_on_overflow( true ),
      use_steps( false ),
      loda_config( "loda.json" ),
      miner( "default" ),
      print_as_b_file( false ),
      print_as_b_file_offset( 0 ),
      printed_memory_warning( false )
{
}

enum class Option
{
  NONE,
  NUM_TERMS,
  MAX_MEMORY,
  MAX_CYCLES,
  MAX_PHYSICAL_MEMORY,
  B_FILE_OFFSET,
  LODA_CONFIG,
  MINER,
  LOG_LEVEL
};

std::vector<std::string> Settings::parseArgs( int argc, char *argv[] )
{
  Option option( Option::NONE );
  std::vector<std::string> unparsed;
  for ( int i = 1; i < argc; ++i )
  {
    std::string arg( argv[i] );
    if ( option == Option::NUM_TERMS || option == Option::MAX_MEMORY || option == Option::MAX_CYCLES
        || option == Option::MAX_PHYSICAL_MEMORY || option == Option::B_FILE_OFFSET )
    {
      std::stringstream s( arg );
      int64_t val;
      s >> val;
      if ( option != Option::MAX_CYCLES && option != Option::B_FILE_OFFSET && val < 1 )
      {
        Log::get().error( "Invalid value for option: " + std::to_string( val ), true );
      }
      switch ( option )
      {
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
      case Option::MAX_PHYSICAL_MEMORY:
        max_physical_memory = val * 1024 * 1024;
        break;
      case Option::LOG_LEVEL:
      case Option::LODA_CONFIG:
      case Option::MINER:
      case Option::NONE:
        break;
      }
      option = Option::NONE;
    }
    else if ( option == Option::LODA_CONFIG )
    {
      loda_config = arg;
      option = Option::NONE;
    }
    else if ( option == Option::MINER )
    {
      miner = arg;
      option = Option::NONE;
    }
    else if ( option == Option::LOG_LEVEL )
    {
      if ( arg == "debug" )
      {
        Log::get().level = Log::Level::DEBUG;
      }
      else if ( arg == "info" )
      {
        Log::get().level = Log::Level::INFO;
      }
      else if ( arg == "warn" )
      {
        Log::get().level = Log::Level::WARN;
      }
      else if ( arg == "error" )
      {
        Log::get().level = Log::Level::ERROR;
      }
      else if ( arg == "alert" )
      {
        Log::get().level = Log::Level::ALERT;
      }
      else
      {
        Log::get().error( "Unknown log level: " + arg );
      }
      option = Option::NONE;
    }
    else if ( arg.at( 0 ) == '-' )
    {
      std::string opt = arg.substr( 1 );
      if ( opt == "t" )
      {
        option = Option::NUM_TERMS;
      }
      else if ( opt == "m" )
      {
        option = Option::MAX_MEMORY;
      }
      else if ( opt == "c" )
      {
        option = Option::MAX_CYCLES;
      }
      else if ( opt == "p" )
      {
        option = Option::MAX_PHYSICAL_MEMORY;
      }
      else if ( opt == "i" )
      {
        option = Option::MINER;
      }
      else if ( opt == "r" )
      {
        search_linear = true;
      }
      else if ( opt == "s" )
      {
        use_steps = true;
      }
      else if ( opt == "b" )
      {
        option = Option::B_FILE_OFFSET;
      }
      else if ( opt == "k" )
      {
        option = Option::LODA_CONFIG;
      }
      else if ( opt == "l" )
      {
        option = Option::LOG_LEVEL;
      }
      else
      {
        Log::get().error( "Unknown option: -" + opt, true );
      }
    }
    else
    {
      unparsed.push_back( arg );
    }
  }
  return unparsed;
}

bool Settings::hasMemory() const
{
  bool has_memory = getMemUsage() <= (size_t) (0.95 * max_physical_memory);
  if ( !has_memory && !printed_memory_warning )
  {
    Log::get().warn(
        "Reached maximum physical memory limit of " + std::to_string( max_physical_memory / (1024 * 1024) ) + "MB" );
    printed_memory_warning = true;
  }
  return has_memory;
}

std::string getLodaHome()
{
  // don't remove the trailing /
  return std::string( std::getenv( "HOME" ) ) + "/.loda/";
}

void ensureDir( const std::string &path )
{
  auto index = path.find_last_of( "/" );
  if ( index != std::string::npos )
  {
    auto dir = path.substr( 0, index );
    auto cmd = "mkdir -p " + dir;
    auto exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error creating directory " + dir, true );
    }
  }
  else
  {
    Log::get().error( "Error determining directory for " + path, true );
  }
}

int64_t getFileAgeInDays( const std::string &path )
{
  struct stat st;
  if ( stat( path.c_str(), &st ) == 0 )
  {
    time_t now = time( 0 );
    return (now - st.st_mtime) / (3600 * 24);
  }
  return -1;
}

size_t getMemUsage()
{
  size_t mem_usage = 0;
#if __linux__
  auto fp = fopen( "/proc/self/statm", "r" );
  if ( fp )
  {
    long rss = 0;
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 )
    {
      rss = 0;
    }
    fclose( fp );
    mem_usage = (size_t) (rss) * (size_t) (sysconf( _SC_PAGE_SIZE ));
  }
#elif __MACH__
  mach_msg_type_number_t cnt = MACH_TASK_BASIC_INFO_COUNT;
  mach_task_basic_info info;
  if ( task_info( mach_task_self( ), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>( &info ),
      &cnt ) == KERN_SUCCESS )
  {
    mem_usage = info.resident_size;
  }
#endif
  return mem_usage;
}

FolderLock::FolderLock( std::string folder )
{
  // obtain lock
  if ( folder[folder.size() - 1] != '/' )
  {
    folder += '/';
  }
  ensureDir( folder );
  lockfile = folder + "lock";
  fd = 0;
  Log::get().debug( "Acquiring lock " + lockfile );
  while ( true )
  {
    fd = open( lockfile.c_str(), O_CREAT, 0644 );
    flock( fd, LOCK_EX );
    struct stat st0, st1;
    fstat( fd, &st0 );
    stat( lockfile.c_str(), &st1 );
    if ( st0.st_ino == st1.st_ino ) break;
    close( fd );
  }
  Log::get().debug( "Obtained lock " + lockfile );
}

FolderLock::~FolderLock()
{
  release();
}

void FolderLock::release()
{
  if ( fd )
  {
    Log::get().debug( "Releasing lock " + lockfile );
    unlink( lockfile.c_str() );
    flock( fd, LOCK_UN );
    fd = 0;
  }
}

AdaptiveScheduler::AdaptiveScheduler( int64_t target_seconds )
    : target_seconds( target_seconds )
{
  reset();
}

bool AdaptiveScheduler::isTargetReached()
{
  total_checks++;
  if ( total_checks >= next_check )
  {
    auto cur_time = std::chrono::steady_clock::now();
    int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>( cur_time - start_time ).count();
    if ( seconds >= target_seconds )
    {
      return true;
    }
    if ( 2 * seconds < target_seconds )
    {
      next_check *= 2;
    }
    else
    {
      double speed = static_cast<double>( total_checks ) / static_cast<double>( std::max<int64_t>( seconds, 1 ) );
      next_check += std::max<int64_t>( static_cast<int64_t>( (target_seconds - seconds) * speed ), 1 );
    }
  }
  return false;
}

void AdaptiveScheduler::reset()
{
  total_checks = 0;
  next_check = 1;
  start_time = std::chrono::steady_clock::now();
}
