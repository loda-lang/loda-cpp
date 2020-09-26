#pragma once

#include <array>
#include <iostream>
#include <unordered_map>

#include "number.hpp"

#define MEMORY_CACHE_SIZE 16

class Memory
{
public:

  Memory();

  number_t get( number_t i ) const;

  void set( number_t i, number_t v );

  void clear();

  void clear( number_t start, int64_t length );

  Memory fragment( number_t start, int64_t length, bool normalize ) const;

  size_t approximate_size() const;

  bool is_less( const Memory &m, int64_t length ) const;

  bool operator==( const Memory &m ) const;

  bool operator!=( const Memory &m ) const;

  friend std::ostream& operator<<( std::ostream &out, const Memory &m );

private:

  std::array<number_t, MEMORY_CACHE_SIZE> cache;
  std::unordered_map<number_t, number_t> full;

};
