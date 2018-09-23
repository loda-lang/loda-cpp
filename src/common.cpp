#include "common.hpp"

// === Sequence ===============================================================

bool Sequence::operator<( const Sequence& other ) const
{
  number_t length = size() < other.size() ? size() : other.size();
  for ( number_t i = 0; i < length; i++ )
  {
    if ( (*this)[i] < other[i] )
    {
      return true; // less
    }
    else if ( (*this)[i] > other[i] )
    {
      return false; // greater
    }
  }
  return false; // undecidable
}

bool Sequence::operator!=( const Sequence& other ) const
{
  number_t length = size() < other.size() ? size() : other.size();
  for ( number_t i = 0; i < length; i++ )
  {
    if ( (*this)[i] != other[i] )
    {
      return true; // not equal
    }
  }
  return false; // undecidable
}

std::ostream& operator<<( std::ostream& out, const Sequence& seq )
{
  for ( number_t i = 0; i < seq.size(); i++ )
  {
    if ( i > 0 ) out << " ";
    out << seq[i];
  }
  return out;
}

// === Memory =================================================================

number_t Memory::get( number_t index ) const
{
  if ( index >= size() )
  {
    return 0;
  }
  return (*this)[index];
}

void Memory::set( number_t index, number_t value )
{
  if ( index > 10000 )
  {
    throw std::runtime_error( "index out of loda memory range: " + std::to_string( index ) );
  }
  if ( index >= size() )
  {
    resize( index + 1, 0 );
  }
  (*this)[index] = value;
}

Memory Memory::fragment( number_t start, number_t length ) const
{
  Memory f;
  for ( number_t i = 0; i < length; i++ )
  {
    f.set( i, get( start + i ) );
  }
  return f;
}

bool Memory::operator<( const Memory& other ) const
{
  number_t length = size() > other.size() ? size() : other.size();
  for ( number_t i = 0; i < length; i++ )
  {
    if ( get( i ) < other.get( i ) )
    {
      return true; // less
    }
    else if ( get( i ) > other.get( i ) )
    {
      return false; // greater
    }
  }
  return false; // equal
}

bool Memory::operator==( const Memory& other ) const
{
  number_t length = size() > other.size() ? size() : other.size();
  for ( number_t i = 0; i < length; i++ )
  {
    if ( get( i ) != other.get( i ) )
    {
      return false;
    }
  }
  return true; // equal
}

bool Memory::operator!=( const Memory& other ) const
{
  return !(*this == other);
}

std::ostream& operator<<( std::ostream& out, const Memory& m )
{
  out << "[";
  for ( number_t i = 0; i < m.size(); i++ )
  {
    if ( i > 0 ) out << ",";
    out << m[i];
  }
  out << "]";
  return out;
}
