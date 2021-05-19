#pragma once

#include "sequence.hpp"

struct delta_t
{
  number_t delta;
  number_t offset;
  number_t factor;
};

class Reducer
{
public:

  static number_t truncate( Sequence &seq );

  static number_t shrink( Sequence &seq );

  static delta_t delta( Sequence &seq, int64_t max_delta );

  static int64_t digit( Sequence &seq, int64_t num_digits );

};
