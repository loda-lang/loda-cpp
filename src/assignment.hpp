#pragma once

#include <stdexcept>

class Assignment
{
public:

  static constexpr uint8_t DIFF_BITMASK = 0b10000000;
  static constexpr uint8_t SIGN_BITMASK = 0b01000000;
  static constexpr uint8_t VALUE_BITMASK = 0b00111111;

  inline Assignment( int64_t value, bool is_diff )
  {
    if ( value > VALUE_BITMASK || value < -VALUE_BITMASK )
    {
      throw std::invalid_argument( "value out of range" );
    }
    data = value & VALUE_BITMASK;
    if ( data < 0 )
    {
      data = data | SIGN_BITMASK;
    }
    if ( is_diff )
    {
      data = data | DIFF_BITMASK;
    }
  }

  inline bool isDiff() const
  {
    return data & DIFF_BITMASK;
  }

  inline int8_t getValue() const
  {
    return (data & SIGN_BITMASK) ? -(data & VALUE_BITMASK) : (data & VALUE_BITMASK);
  }

private:
  uint8_t data;
};
