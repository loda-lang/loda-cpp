#pragma once

#include "number.hpp"

#include <unordered_map>
#include <vector>

class Sequence: public std::vector<number_t>
{
public:

  Sequence() = default;

  Sequence( const Sequence &s ) = default;

  Sequence( const std::vector<number_t> &s )
      : std::vector<number_t>( s )
  {
  }

  Sequence subsequence( size_t start ) const;

  Sequence subsequence( size_t start, size_t length ) const;

  bool is_linear( size_t start ) const;

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
