#include "number.hpp"

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
