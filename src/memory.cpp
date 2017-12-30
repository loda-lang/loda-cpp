#include "memory.hpp"

#include "value.hpp"

Memory::Memory()
{
  regs.fill( 0 );
}

bool Memory::IsLessThan( const Memory& other, const std::vector<Value> cmp_vars )
{
  for ( Value v : cmp_vars )
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

std::array<Value, 256> regs;

std::ostream& operator<<( std::ostream& out, const Memory& m )
{
  size_t last = m.regs.size() - 1;
  while ( last > 1 && m.regs[last] == 0 )
  {
    last--;
  }
  for ( size_t i = 0; i <= last; i++ )
  {
    if ( i > 0 ) out << ",";
    out << m.regs[i];
  }
  return out;
}
