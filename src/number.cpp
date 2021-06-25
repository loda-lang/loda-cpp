#include "number.hpp"

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

int64_t Number::asInt() const
{
  if ( is_big )
  {
    throw std::runtime_error( "Bigint not supported yet" );
  }
  // TODO: throw an exception if the value is out of range
  return value;
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
