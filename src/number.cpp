#include "number.hpp"

#include <sstream>
#include <stdexcept>

const Number Number::ZERO( 0 );
const Number Number::ONE( 1 );
const Number Number::INF( NUM_INF );

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

Number::Number( std::istream& in, bool is_big )
{
  load( in, is_big );
}

Number::Number( const std::string& s, bool is_big )
{
  std::stringstream buf( s );
  load( buf, is_big );
}

void Number::load( std::istream& in, bool is_big )
{
  this->is_big = is_big;
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  else
  {
    in >> value;
    if ( value == std::numeric_limits<int64_t>::max() || value == std::numeric_limits<int64_t>::min() )
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

bool isCloseToOverflow( number_t n )
{
  return (n > (NUM_INF / 1000)) || (n < (NUM_INF / -1000));
}

number_t getPowerOf( number_t value, number_t base )
{
  number_t result = 0;
  while ( value % base == 0 )
  {
    result++;
    value /= base;
  }
  return (value == 1) ? result : 0;
}
