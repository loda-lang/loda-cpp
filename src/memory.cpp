#include "memory.hpp"

#include "types.hpp"

Memory::Memory()
{
  regs.fill( 0 );
}

bool Memory::IsLessThan( const Memory& other, const std::vector<var_t> cmp_vars )
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

std::ostream& operator<<( std::ostream& os, const Memory& m )
{
  size_t last = m.regs.size() - 1;
  while ( last > 1 && m.regs[last] == 0 )
  {
    last--;
  }
  for ( size_t i = 0; i <= last; i++ )
  {
    if ( i > 0 ) os << ",";
    os << m.regs[i];
  }
  return os;
}
