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

  number_t Get( number_t i ) const;

  void Set( number_t i, number_t v );

  number_t Length() const;

  Sequence Fragment( number_t start, number_t length ) const;

  bool operator<( const Sequence& other ) const;

  bool operator==( const Sequence& other ) const;

  bool operator!=( const Sequence& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& m );

//private:

  std::vector<number_t> data;

};

