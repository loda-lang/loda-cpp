#pragma once

#include <array>
#include <iostream>
#include <stdlib.h>

class BigNumber
{
public:

  static constexpr size_t NUM_WORDS = 10;
  static constexpr size_t NUM_WORD_DIGITS = 18;
  static constexpr size_t NUM_DIGITS = NUM_WORDS * NUM_WORD_DIGITS;
  static constexpr uint64_t WORD_BASE = 1000000000000000000;

  BigNumber();

  BigNumber( int64_t value );

  BigNumber( const std::string& s );

  BigNumber& negate();

  friend std::ostream& operator<<( std::ostream &out, const BigNumber &n );

  inline bool isNegative() const
  {
    return is_negative;
  }

  inline bool isInfinite() const
  {
    return is_infinite;
  }

private:

  std::array<uint64_t, NUM_WORDS> words;
  bool is_negative;
  bool is_infinite;

};
