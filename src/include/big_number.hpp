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

  BigNumber& negate();

  bool parse( const std::string& s );

  void print( std::ostream & out );

private:

  std::array<uint64_t, NUM_WORDS> words;
  bool is_negative;

};
