#pragma once

#include <stdlib.h>
#include <iostream>
#include <random>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using number_t = uint64_t;

class Sequence: public std::vector<number_t>
{
public:

  Sequence() = default;

  Sequence( const Sequence& s ) = default;

  Sequence( const std::vector<number_t>& s )
      : std::vector<number_t>( s )
  {
  }

  Sequence subsequence( size_t start );

  bool is_linear() const;

  size_t distinct_values() const;

  number_t min() const;

  void add( number_t n );

  void sub( number_t n );

  bool operator<( const Sequence& s ) const;

  bool operator!=( const Sequence& s ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& s );

  std::string to_string() const;

};

struct SequenceHasher
{
  std::size_t operator()( const Sequence& s ) const
  {
    std::size_t seed = s.size();
    for ( auto& i : s )
    {
      seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

class SequenceToIdsMap: public std::unordered_map<Sequence, std::vector<number_t>, SequenceHasher>
{
public:
  void remove( Sequence seq, number_t id );
};

class Memory: public std::vector<number_t>
{
public:

  Memory() = default;

  Memory( const Memory& m ) = default;

  Memory( const std::vector<number_t>& m )
      : std::vector<number_t>( m )
  {
  }

  number_t get( number_t i ) const;

  void set( number_t i, number_t v );

  Memory fragment( number_t start, number_t length ) const;

  bool operator<( const Memory& m ) const;

  bool operator==( const Memory& m ) const;

  bool operator!=( const Memory& m ) const;

  friend std::ostream& operator<<( std::ostream& out, const Memory& m );

};

class Distribution: public std::discrete_distribution<>
{
public:

  Distribution( const std::vector<double>& probabilities );

  static Distribution uniform( size_t size );

  static Distribution exponential( size_t size );

  static Distribution add( const Distribution& d1, const Distribution& d2 );

  static Distribution mutate( const Distribution& d, std::mt19937& gen );

  void print() const;

};
