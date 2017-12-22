#pragma once

#include "program.hpp"

template<size_t NUM_VARS>
class Interpreter
{
  using State = std::array<uint64_t,NUM_VARS>;

  void Apply( const Assignment::Array<NUM_VARS>& a, State& s )
  {
    for ( size_t i = 0; i < NUM_VARS; i++ )
    {
      s[i] = a[i].isReset() ? a[i].getValue() : s[i] + a[i].getValue();
    }
  }

  bool Run( const Program<NUM_VARS>& p, State& s )
  {

    return true;
  }

};
