#pragma once

class Assignment
{
public:

  static constexpr uint8_t DIFF_BITMASK = 0b10000000;
  static constexpr uint8_t SIGN_BITMASK = 0b01000000;
  static constexpr uint8_t VALUE_BITMASK = 0b00111111;

  inline Assignment( bool is_diff, int8_t value )
  {
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
