#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <stdlib.h>

using number_t = int64_t;

static constexpr number_t NUM_INF = std::numeric_limits<number_t>::max();

inline bool isCloseToOverflow( number_t n )
{
  return (n > (NUM_INF / 1000)) || (n < (NUM_INF / -1000));
}
