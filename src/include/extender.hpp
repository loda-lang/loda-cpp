#pragma once

#include "program.hpp"
#include "sequence.hpp"

struct line_t
{
  Number offset;
  Number factor;
};

class Extender
{
public:

  static bool linear1( Program &p, line_t inverse, line_t target );

  static bool linear2( Program &p, line_t inverse, line_t target );

  static bool delta_one( Program &p, const bool sum );

  static bool delta_it( Program &p, int64_t delta );

  static bool digit( Program &p, int64_t num_digits, int64_t offset );

};
