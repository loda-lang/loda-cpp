#pragma once

#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <random>
#include <memory>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

using number_t = int64_t;

static constexpr number_t NUM_INF = std::numeric_limits<number_t>::max();

inline bool isCloseToOverflow( number_t n )
{
  return (n > (NUM_INF / 1000)) || (n < (NUM_INF / -1000));
}

class Sequence: public std::vector<number_t>
{
public:

  Sequence() = default;

  Sequence( const Sequence &s ) = default;

  Sequence( const std::vector<number_t> &s )
      :
      std::vector<number_t>( s )
  {
  }

  Sequence subsequence( size_t start ) const;

  Sequence subsequence( size_t start, size_t length ) const;

  bool is_linear() const;

  size_t distinct_values() const;

  number_t min( bool includeNegative ) const;

  void add( number_t n );

  void sub( number_t n );

  number_t sum() const;

  bool align( const Sequence &s, int64_t max_offset );

  bool operator<( const Sequence &s ) const;

  bool operator==( const Sequence &s ) const;

  bool operator!=( const Sequence &s ) const;

  friend std::ostream& operator<<( std::ostream &out, const Sequence &s );

  std::string to_string() const;

};

class Polynomial: public std::vector<int64_t>
{
public:

  Polynomial() = default;

  Polynomial( const Polynomial &s ) = default;

  Polynomial( size_t degree )
      :
      std::vector<int64_t>( degree + 1 )
  {
  }

  Polynomial( const std::vector<int64_t> &p )
      :
      std::vector<int64_t>( p )
  {
  }

  Sequence eval( number_t length ) const;

  std::string to_string() const;

  friend Polynomial operator+( const Polynomial &a, const Polynomial &b );
  friend Polynomial operator-( const Polynomial &a, const Polynomial &b );

};

struct SequenceHasher
{
  std::size_t operator()( const Sequence &s ) const
  {
    std::size_t seed = s.size();
    for ( auto &i : s )
    {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

class SequenceToIdsMap: public std::unordered_map<Sequence, std::vector<size_t>, SequenceHasher>
{
public:
  void remove( Sequence seq, size_t id );
};

#define MEMORY_CACHE_SIZE 16

class Memory
{
public:

  Memory() = default;

  Memory( const Memory &m ) = default;

  void clear();

  number_t get( number_t i ) const;

  void set( number_t i, number_t v );

  Memory fragment( number_t start, size_t length ) const;

  bool is_less( const Memory &m, size_t length ) const;

  bool operator==( const Memory &m ) const;

  bool operator!=( const Memory &m ) const;

  friend std::ostream& operator<<( std::ostream &out, const Memory &m );

private:

  std::array<number_t, MEMORY_CACHE_SIZE> cache;
  std::unordered_map<number_t, number_t> full;

};
