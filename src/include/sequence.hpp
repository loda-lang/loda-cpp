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

  Sequence subsequence( size_t start, size_t length ) const;

  bool is_linear( size_t start ) const;

  bool align( const Sequence &s, int64_t max_offset );

  bool operator==( const Sequence &s ) const;

  bool operator!=( const Sequence &s ) const;

  friend std::ostream& operator<<( std::ostream &out, const Sequence &s );

  std::string to_string() const;

};

struct SequenceHasher
{
  std::size_t operator()( const Sequence &s ) const;
};

class SequenceToIdsMap: public std::unordered_map<Sequence, std::vector<size_t>, SequenceHasher>
{
public:
  void remove( Sequence seq, size_t id );
};
