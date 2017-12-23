#pragma once

#include <array>

#include "types.hpp"

class Memory
{
public:

  Memory()
  {
    regs.fill( 0 );
  }

  bool IsLessThan( const Memory& other, const std::vector<var_t> cmp_vars )
  {
    for ( var_t v : cmp_vars )
    {
      if ( regs[v] < other.regs[v] )
      {
        return true; // less
      }
      else if ( regs[v] > other.regs[v] )
      {
        return false; // greater
      }
    }
    return false; // equal
  }

  std::array<value_t, 256> regs;

};

std::ostream& operator<<( std::ostream& os, const Memory& m )
{
  for ( size_t i = 0; i < m.regs.size(); i++ )
  {
    if ( i > 0 ) os << ",";
    os << m.regs[i];
  }
  return os;
}
