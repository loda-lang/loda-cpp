#pragma once

#include "number.hpp"
#include "program.hpp"

struct line_t
{
  int offset;
  int factor;
};

class Extender
{
public:

  static bool linear1( Program &p, line_t inverse, line_t target );

  static bool linear2( Program &p, line_t inverse, line_t target );

  static bool polynomial( Program &p, const Polynomial &diff );

  static bool delta_one( Program &p, const bool sum );

  static bool delta_it( Program &p, int delta );

};
