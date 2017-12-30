#include "memory.hpp"

#include "value.hpp"

Memory::Memory()
{
}

Value Memory::Get( Value i ) const
{
  if ( i >= data.size() )
  {
    return 0;
  }
  return data[i];
}

void Memory::Set( Value i, Value v )
{
  if ( i >= data.size() )
  {
    data.resize( i + 1, 0 );
  }
  data[i] = v;
}

Value Memory::Length() const
{
  return data.size();
}

bool Memory::operator<( const Memory& other ) const
{
  Value length = Length() > other.Length() ? Length() : other.Length();
  for ( Value i = 0; i < length; i++ )
  {
    if ( Get( i ) < other.Get( i ) )
    {
      return true; // less
    }
    else if ( Get( i ) > other.Get( i ) )
    {
      return false; // greater
    }
  }
  return false; // equal
}

Memory Memory::Fragment( Value start, Value length ) const
{
  Memory f;
  for ( Value i = 0; i < length; i++ )
  {
    f.Set( i, Get( start + i ) );
  }
  return f;
}

std::ostream& operator<<( std::ostream& out, const Memory& m )
{
  for ( size_t i = 0; i < m.data.size(); i++ )
  {
    if ( i > 0 ) out << ",";
    out << m.data[i];
  }
  return out;
}
