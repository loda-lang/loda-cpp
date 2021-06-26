#pragma once

#include "sequence.hpp"

struct delta_t
{
  int64_t delta;
  Number offset;
  Number factor;
};

class Reducer
{
public:

  static Number truncate( Sequence &seq );

  static Number shrink( Sequence &seq );

  static delta_t delta( Sequence &seq, int64_t max_delta );

  static int64_t digit( Sequence &seq, int64_t num_digits );

};
