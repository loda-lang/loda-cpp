#include "number.hpp"

Number::Number()
    : value( 0 )
{
}

Number::Number( int64_t value )
    : value( value )
{
}

int64_t Number::asInt() const
{
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
