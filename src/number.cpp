#include "number.hpp"

#include <sstream>
#include <stdexcept>

#define NUM_INF std::numeric_limits<int64_t>::max()

const Number Number::ZERO( 0 );
const Number Number::ONE( 1 );
const Number Number::INF( NUM_INF);

Number::Number()
    : value( 0 ),
      is_big( false )
{
}

Number::Number( int64_t value )
    : value( value ),
      is_big( false )
{
}

Number::Number( const std::string& s, bool is_big )
    : is_big( is_big )
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  else
  {
    bool is_inf = false;
    try
    {
      value = std::stoll( s );
    }
    catch ( const std::out_of_range& )
    {
      is_inf = true;
    }
    if ( value == std::numeric_limits<int64_t>::max() || value == std::numeric_limits<int64_t>::min() )
    {
      is_inf = true;
    }
    if ( is_inf )
    {
      (*this) = Number::INF;
    }
  }
}

bool Number::operator==( const Number&n ) const
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  return value == n.value;
}

bool Number::operator!=( const Number&n ) const
{
  return !(*this == n);
}

bool Number::operator<( const Number&n ) const
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  return value < n.value;
}

int64_t Number::asInt() const
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  if ( (*this) == Number::INF )
  {
    throw std::runtime_error( "Infinity error" );
  }
  // TODO: throw an exception if the value is out of range
  return value;
}

std::size_t Number::hash() const
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  return value;
}

std::ostream& operator<<( std::ostream &out, const Number &n )
{
  if ( n.is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  out << n.value;
  return out;
}

std::string Number::to_string() const
{
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void throwParseError( std::istream& in )
{
  std::string tmp;
  std::getline( in, tmp );
  throw std::runtime_error( "Error parsing number: '" + tmp + "'" );
}

void Number::readIntString( std::istream& in, std::string& out )
{
  out.clear();
  auto ch = in.peek();
  if ( !std::isdigit( ch ) && ch != '-' )
  {
    throwParseError( in );
  }
  out += (char) ch;
  in.get();
  while ( true )
  {
    auto ch = in.peek();
    if ( !std::isdigit( ch ) )
    {
      break;
    }
    out += (char) ch;
    in.get();
  }
}
