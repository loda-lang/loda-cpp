#pragma once

#include "number.hpp"

struct delta_t
{
  int delta;
  int offset;
  int factor;
};

class Reducer
{
public:

  static int truncate( Sequence &seq );

  static int shrink( Sequence &seq );

  static Polynomial polynomial( Sequence &seq, int degree );

  static delta_t delta( Sequence &seq, int max_delta );

};
