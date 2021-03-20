#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <stdlib.h>

using number_t = int64_t;

static constexpr number_t NUM_INF = std::numeric_limits<number_t>::max();

bool isCloseToOverflow( number_t n );

number_t getPowerOf( number_t value, number_t base );
