#include "util.hpp"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

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
  if ( system( "twidge -h > /dev/null" ) == 0 )
  {
    return TW_TWIDGE;
  }
  if ( system( "rainbowstream -h > /dev/null" ) == 0 )
  {
    return TW_RAINBOWSTREAM;
  }
  return TW_NONE;
}

Log::Log()
    :
    level( Level::INFO ),
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

void Log::alert( const std::string &msg )
{
  log( Log::Level::ALERT, msg );
  std::string copy = msg;
  if ( copy.length() > 140 )
  {
    copy = copy.substr( 0, 137 );
    std::replace( copy.begin(), copy.end(), '"', '\'' );
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
  if ( tweet_alerts && !copy.empty() )
  {
    if ( twitter_client == TW_UNKNOWN )
    {
      twitter_client = findTwitterClient();
    }
    std::string cmd;
    switch ( twitter_client )
    {
    case TW_UNKNOWN:
    case TW_NONE:
      break;
    case TW_TWIDGE:
    {
      cmd = "twidge update \"" + copy + "\" > /dev/null";
      break;
    }
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
{
  auto h = std::getenv( "LODA_INFLUXDB_HOST" );
  if ( h )
  {
    host = std::string( h );
    Log::get().info( "Publishing metrics to InfluxDB at " + host );
  }
}

Metrics& Metrics::get()
{
  static Metrics metrics;
  return metrics;
}

void Metrics::write( const std::string &field, const std::map<std::string, std::string> &labels, double value ) const
{
  if ( Metrics::host.empty() )
  {
    return;
  }
  std::stringstream buf;
  buf << "curl -i -s -XPOST '" << host << "/write?db=loda' --data-binary '" << field;
  for ( auto &l : labels )
  {
    buf << "," << l.first << "=" << l.second;
  }
  buf << " value=" << value << "' > /dev/null";
  auto cmd = buf.str();
  auto exit_code = system( cmd.c_str() );
  if ( exit_code != 0 )
  {
    Log::get().error( "Error publishing metrics; error code " + std::to_string( exit_code ), false );
  }
}

Settings::Settings()
    :
    num_terms( 20 ),
    num_operations( 20 ),
    max_memory( getEnvInt( "LODA_MAX_MEMORY", 100000 ) ),
    max_cycles( getEnvInt( "LODA_MAX_CYCLES", 10000000 ) ),
    max_constant( 6 ),
    max_index( 6 ),
    max_stack_size( getEnvInt( "LODA_MAX_STACK_SIZE", 100 ) ),
    dump_last_program( getEnvFlag( "LODA_DUMP_LAST_PROGRAM" ) ),
    optimize_existing_programs( false ),
    search_linear( false ),
    throw_on_overflow( true ),
    operation_types( "^" ),
    operand_types( "cd" )
{
}

enum class Option
{
  NONE,
  NUM_TERMS,
  NUM_OPERATIONS,
  MAX_MEMORY,
  MAX_CYCLES,
  MAX_CONSTANT,
  MAX_INDEX,
  OPERATION_TYPES,
  OPERAND_TYPES,
  PROGRAM_TEMPLATE,
  LOG_LEVEL
};

std::vector<std::string> Settings::parseArgs( int argc, char *argv[] )
{
  Option option( Option::NONE );
  std::vector<std::string> unparsed;
  for ( int i = 1; i < argc; ++i )
  {
    std::string arg( argv[i] );
    if ( option == Option::NUM_TERMS || option == Option::NUM_OPERATIONS || option == Option::MAX_MEMORY
        || option == Option::MAX_CYCLES || option == Option::MAX_CONSTANT || option == Option::MAX_INDEX )
    {
      std::stringstream s( arg );
      int val;
      s >> val;
      if ( val < 1 )
      {
        Log::get().error( "Invalid value for option: " + std::to_string( val ), true );
      }
      switch ( option )
      {
      case Option::NUM_TERMS:
        num_terms = val;
        break;
      case Option::NUM_OPERATIONS:
        num_operations = val;
        break;
      case Option::MAX_MEMORY:
        max_memory = val;
        break;
      case Option::MAX_CYCLES:
        max_cycles = val;
        break;
      case Option::MAX_CONSTANT:
        max_constant = val;
        break;
      case Option::MAX_INDEX:
        max_index = val;
        break;
      case Option::LOG_LEVEL:
      case Option::OPERATION_TYPES:
      case Option::OPERAND_TYPES:
      case Option::PROGRAM_TEMPLATE:
      case Option::NONE:
        break;
      }
      option = Option::NONE;
    }
    else if ( option == Option::OPERATION_TYPES )
    {
      operation_types = arg;
      option = Option::NONE;
    }
    else if ( option == Option::OPERAND_TYPES )
    {
      operand_types = arg;
      option = Option::NONE;
    }
    else if ( option == Option::PROGRAM_TEMPLATE )
    {
      program_template = arg;
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
      else if ( opt == "p" )
      {
        option = Option::NUM_OPERATIONS;
      }
      else if ( opt == "m" )
      {
        option = Option::MAX_MEMORY;
      }
      else if ( opt == "c" )
      {
        option = Option::MAX_CYCLES;
      }
      else if ( opt == "n" )
      {
        option = Option::MAX_CONSTANT;
      }
      else if ( opt == "i" )
      {
        option = Option::MAX_INDEX;
      }
      else if ( opt == "x" )
      {
        optimize_existing_programs = true;
      }
      else if ( opt == "r" )
      {
        search_linear = true;
      }
      else if ( opt == "o" )
      {
        option = Option::OPERATION_TYPES;
      }
      else if ( opt == "a" )
      {
        option = Option::OPERAND_TYPES;
      }
      else if ( opt == "e" )
      {
        option = Option::PROGRAM_TEMPLATE;
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
