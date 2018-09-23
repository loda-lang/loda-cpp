#pragma once

#include <stdlib.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using number_t = uint64_t;

class Sequence
{
public:

  Sequence();

  number_t get( number_t i ) const;

  void set( number_t i, number_t v );

  number_t size() const;

  Sequence fragment( number_t start, number_t length ) const;

  bool operator<( const Sequence& other ) const;

  bool operator==( const Sequence& other ) const;

  bool operator!=( const Sequence& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& m );

//private:

  std::vector<number_t> data;

};

