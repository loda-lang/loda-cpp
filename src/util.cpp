#include "util.hpp"

#include <ctime>
#include <iostream>

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
  time_t rawtime;
  char buffer[80];
  time( &rawtime );
  auto timeinfo = localtime( &rawtime );
  strftime( buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo );
  std::string lev;
  switch ( level )
  {
  case Log::Level::DEBUG:
    lev = "DEBUG";
    break;
  case Log::Level::INFO:
    lev = "INFO";
    break;
  case Log::Level::WARN:
    lev = "WARN";
    break;
  case Log::Level::ERROR:
    lev = "ERROR";
    break;
  }
  std::cerr << std::string( buffer ) << "|" << lev << "|" << msg << std::endl;
}
