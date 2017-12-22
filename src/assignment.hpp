#pragma once

#include <array>
#include <stdexcept>

class Assignment
{
public:

  // array type
  template<std::size_t N>
  using Array = std::array<Assignment,N>;

  // bitmasks
  static constexpr uint8_t RESET_BITMASK = 0b10000000;
  static constexpr uint8_t SIGN_BITMASK = 0b01000000;
  static constexpr uint8_t VALUE_BITMASK = 0b00111111;

  Assignment()
      : data( 0 ) // diff 0
  {
  }

  Assignment( int64_t value, bool is_reset )
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
    if ( is_reset )
    {
      data = data | RESET_BITMASK;
    }
  }

  inline bool isReset() const
  {
    return data & RESET_BITMASK;
  }

  inline int8_t getValue() const
  {
    return (data & SIGN_BITMASK) ? -(data & VALUE_BITMASK) : (data & VALUE_BITMASK);
  }

private:
  uint8_t data;
};
