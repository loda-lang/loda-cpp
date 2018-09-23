#pragma once

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>
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

  bool operator<( const Sequence& s ) const;

  bool operator!=( const Sequence& s ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& s );

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
