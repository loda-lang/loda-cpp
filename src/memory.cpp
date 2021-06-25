#include "memory.hpp"

#include "number.hpp"

Memory::Memory()
{
  cache.fill( 0 );
}

number_t Memory::get( int64_t index ) const
{
  if ( index >= 0 && index < MEMORY_CACHE_SIZE )
  {
    return cache[index];
  }
  auto it = full.find( index );
  if ( it != full.end() )
  {
    return it->second;
  }
  return 0;
}

void Memory::set( int64_t index, number_t value )
{
  if ( index >= 0 && index < MEMORY_CACHE_SIZE )
  {
    cache[index] = value;
  }
  else
  {
    if ( value == 0 )
    {
      full.erase( index );
    }
    else
    {
      full[index] = value;
    }
  }
}

void Memory::clear()
{
  cache.fill( 0 );
  full.clear();
}

void Memory::clear( int64_t start, int64_t length )
{
  if ( start == NUM_INF || length <= 0 )
  {
    return;
  }
  auto end = start + length;
  if ( length < MEMORY_CACHE_SIZE )
  {
    for ( int64_t i = start; i < end; i++ )
    {
      set( i, 0 );
    }
  }
  else
  {
    for ( int64_t i = 0; i < (int64_t) MEMORY_CACHE_SIZE; ++i )
    {
      if ( i >= start && i < end )
      {
        cache[i] = 0;
      }
    }
    auto i = full.begin();
    while ( i != full.end() )
    {
      if ( i->first >= start && i->first < end )
      {
        i = full.erase( i );
      }
      else
      {
        i++;
      }
    }
  }
}

Memory Memory::fragment( int64_t start, int64_t length ) const
{
  Memory frag;
  if ( start == NUM_INF || length <= 0 )
  {
    return frag;
  }
  if ( length < MEMORY_CACHE_SIZE )
  {
    for ( int64_t i = 0; i < (int64_t) length; ++i )
    {
      frag.set( i, get( start + i ) );
    }
  }
  else
  {
    auto end = start + length;
    for ( int64_t i = 0; i < MEMORY_CACHE_SIZE; i++ )
    {
      if ( i >= start && i < end )
      {
        frag.set( i - start, cache[i] );
      }
    }
    auto i = full.begin();
    while ( i != full.end() )
    {
      if ( i->first >= start && i->first < end )
      {
        frag.set( i->first - start, i->second );
      }
      i++;
    }
  }
  return frag;
}

size_t Memory::approximate_size() const
{
  return full.size() + MEMORY_CACHE_SIZE;
}

bool Memory::is_less( const Memory &m, int64_t length, bool check_nonn ) const
{
  if ( length <= 0 )
  {
    return false;
  }
  // TODO: this is slow for large lengths
  for ( int64_t i = 0; i < (int64_t) length; ++i )
  {
    auto lhs = get( i );
    if ( check_nonn && lhs < 0 )
    {
      return false;
    }
    auto rhs = m.get( i );
    if ( lhs < rhs )
    {
      return true; // less
    }
    else if ( lhs > rhs )
    {
      return false; // greater
    }
  }
  return false; // equal
}

bool Memory::operator==( const Memory &m ) const
{
  for ( size_t i = 0; i < MEMORY_CACHE_SIZE; i++ )
  {
    if ( cache[i] != m.cache[i] )
    {
      return false;
    }
  }
  for ( auto i : full )
  {
    if ( i.second != 0 )
    {
      auto j = m.full.find( i.first );
      if ( j == m.full.end() || i.second != j->second )
      {
        return false;
      }
    }
  }
  for ( auto i : m.full )
  {
    if ( i.second != 0 )
    {
      auto j = full.find( i.first );
      if ( j == full.end() || i.second != j->second )
      {
        return false;
      }
    }
  }
  return true; // equal
}

bool Memory::operator!=( const Memory &m ) const
{
  return !(*this == m);
}

std::ostream& operator<<( std::ostream &out, const Memory &m )
{
  out << "[";
  for ( size_t i = 0; i < MEMORY_CACHE_SIZE; ++i )
  {
    if ( i > 0 ) out << ",";
    out << m.cache[i];
  }
  if ( !m.full.empty() )
  {
    out << "...";
  }
  out << "]";
  return out;
}
