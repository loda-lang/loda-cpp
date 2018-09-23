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

  Sequence()
  {
  }

  Sequence( const std::vector<number_t>& s )
      : std::vector<number_t>( s )
  {
  }

  bool operator<( const Sequence& other ) const;

  bool operator!=( const Sequence& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& m );

};

class Memory: public std::vector<number_t>
{
public:

  Memory()
  {
  }

  Memory( const std::vector<number_t>& s )
      : std::vector<number_t>( s )
  {
  }

  number_t get( number_t i ) const;

  void set( number_t i, number_t v );

  Memory fragment( number_t start, number_t length ) const;

  bool operator<( const Memory& other ) const;

  bool operator==( const Memory& other ) const;

  bool operator!=( const Memory& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Memory& m );

};
