#include "number.hpp"

domain_t::domain_t( const std::string &str )
{
  auto p1 = str.find_first_of( ';' );
  auto p2 = str.find_last_of( ';' );
  if ( p1 == std::string::npos || p1 == p2 || str.at( 0 ) != '[' || str.at( str.length() - 1 ) != ']' )
  {
    throw std::runtime_error( "error parsing domain: " + str );
  }
  auto x = str.substr( 1, p1 - 1 );
  auto y = str.substr( p1 + 1, p2 - p1 - 1 );
  auto z = str.substr( p2 + 1, str.length() - p2 - 2 );
  min = (x == "?" || x == "!") ? NUM_INF : std::stoll( x );
  max = (y == "?" || y == "!") ? NUM_INF : std::stoll( y );
  maybe_undef = std::stoll( z );
}

std::string domain_t::to_string() const
{
  std::string x = (min == NUM_INF) ? "?" : std::to_string( min );
  std::string y = (max == NUM_INF) ? "?" : std::to_string( max );
  return "[" + x + ";" + y + ";" + std::to_string( maybe_undef ) + "]";
}
