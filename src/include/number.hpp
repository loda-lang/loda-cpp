#pragma once

#include <algorithm>
#include <limits>
#include <random>
#include <stdlib.h>

using number_t = int64_t;

class Number
{
public:

  static const Number ZERO;
  static const Number ONE;
  static const Number INF;

  Number();

  Number( int64_t value );

  Number( std::istream& in, bool is_big );

  Number( const std::string& s, bool is_big );

  bool operator==( const Number&n ) const;

  bool operator!=( const Number&n ) const;

  bool operator<( const Number&n ) const;

  int64_t asInt() const;

  std::size_t hash() const;

  friend std::ostream& operator<<( std::ostream &out, const Number &n );

  std::string to_string() const;

private:

  friend class Semantics;

  void load( std::istream& in, bool is_big );

  int64_t value;
  bool is_big;

};

static constexpr number_t NUM_INF = std::numeric_limits<number_t>::max();

bool isCloseToOverflow( number_t n );

number_t getPowerOf( number_t value, number_t base );
