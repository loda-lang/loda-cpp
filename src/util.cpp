#include "util.hpp"

#include <ctime>
#include <iostream>
#include <sstream>

Log& Log::get()
{
  static Log log;
  return log;
}

void Log::debug( const std::string& msg )
{
  log( Log::Level::DEBUG, msg );
}

void Log::info( const std::string& msg )
{
  log( Log::Level::INFO, msg );
}

void Log::warn( const std::string& msg )
{
  log( Log::Level::WARN, msg );
}

void Log::error( const std::string& msg, bool throw_ )
{
  log( Log::Level::ERROR, msg );
  if ( throw_ )
  {
    throw std::runtime_error( msg );
  }
}

void Log::log( Level level, const std::string& msg )
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
  }
  std::cerr << std::string( buffer ) << "|" << lev << "|" << msg << std::endl;
}

Settings::Settings()
    : num_terms( 40 ),
      num_operations( 40 ),
      max_memory( 10000 ),
      max_cycles( 10000 ),
      max_constant( 4 ),
      max_index( 4 ),
      operation_types( "asml" ),
      operand_types( "cdi" )
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
