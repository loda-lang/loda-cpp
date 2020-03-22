#include "util.hpp"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <stdlib.h>

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
    std::string cmd = "twidge update \"" + copy + "\"";
    auto exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      error( "Error sending alert using twidge: exit code " + std::to_string( exit_code ), false );
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
    max_memory( 100000 ),
    max_cycles( 10000000 ),
    max_constant( 6 ),
    max_index( 6 ),
    optimize_existing_programs( false ),
    search_linear( false ),
      throw_on_overflow( false ),
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

std::string Settings::getGeneratorArgs() const
{
  std::stringstream ss;
  ss << "-p " << num_operations << " -n " << max_constant << " -i " << max_index << " -o " << operation_types << " -a "
      << operand_types;
  if ( !program_template.empty() )
  {
    ss << " -e " << program_template;
  }
  if ( search_linear )
  {
    ss << " -r";
  }
  return ss.str();
}
