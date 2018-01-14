#pragma once

#include "value.hpp"

#include <iostream>
#include <vector>

class Sequence
{
public:

  Sequence();

  Value Get( Value i ) const;

  void Set( Value i, Value v );

  Value Length() const;

  Sequence Fragment( Value start, Value length ) const;

  bool operator<( const Sequence& other ) const;

  bool operator==( const Sequence& other ) const;

  bool operator!=( const Sequence& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Sequence& m );

//private:

  std::vector<Value> data;

};

