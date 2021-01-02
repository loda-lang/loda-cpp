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

class domain_t
{
public:

  domain_t( number_t min, number_t max, bool maybe_undef )
      :
      min( min ),
      max( max ),
      maybe_undef( maybe_undef )
  {
  }

  domain_t()
      :
      domain_t( NUM_INF, NUM_INF, false )
  {
  }

  domain_t( const std::string &str );

  std::string to_string() const;

  inline bool operator==( const domain_t &other ) const
  {
    return (min == other.min && max == other.max && maybe_undef == other.maybe_undef);
  }

  inline bool operator!=( const domain_t &other ) const
  {
    return !(*this == other);
  }

  number_t min;
  number_t max;
  bool maybe_undef;
};
