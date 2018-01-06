#pragma once

#include "value.hpp"

#include <iostream>
#include <vector>

class Memory
{
public:

  Memory();

  Value Get( Value i ) const;

  void Set( Value i, Value v );

  Value Length() const;

  Memory Fragment( Value start, Value length ) const;

  bool operator<( const Memory& other ) const;

  bool operator==( const Memory& other ) const;

  bool operator!=( const Memory& other ) const;

  friend std::ostream& operator<<( std::ostream& out, const Memory& m );

private:

  std::vector<Value> data;

};

