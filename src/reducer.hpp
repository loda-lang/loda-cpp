#pragma once

#include "number.hpp"

class Reducer
{
public:

  static int truncate( Sequence &seq );

  static int shrink( Sequence &seq );

  static Polynomial polynomial( Sequence &seq, int64_t degree );

};
