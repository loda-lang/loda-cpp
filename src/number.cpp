#include "number.hpp"

#include "util.hpp"

#include <sstream>
#include <unordered_set>

// === Sequence ===============================================================

Sequence Sequence::subsequence( size_t start )
{
  return Sequence( std::vector<number_t>( begin() + start, end() ) );
}

bool Sequence::is_linear() const
{
  if ( size() < 3 )
  {
    return true;
  }
  int d = (*this)[1] - (*this)[0];
  for ( size_t i = 2; i < size(); ++i )
  {
    if ( (*this)[i - 1] + d != (*this)[i] )
    {
      return false;
    }
  }
  return true;
}

size_t Sequence::distinct_values() const
{
  std::unordered_set<number_t> values;
  values.insert( begin(), end() );
  return values.size();
}

bool Sequence::operator<( const Sequence& m ) const
{
  number_t length = size() < m.size() ? size() : m.size();
  for ( number_t i = 0; i < length; ++i )
  {
    if ( (*this)[i] < m[i] )
    {
      return true; // less
    }
    else if ( (*this)[i] > m[i] )
    {
      return false; // greater
    }
  }
  return false; // undecidable
}

bool Sequence::operator!=( const Sequence& m ) const
{
  number_t length = size() < m.size() ? size() : m.size();
  for ( number_t i = 0; i < length; ++i )
  {
    if ( (*this)[i] != m[i] )
    {
      return true; // not equal
    }
  }
  return false; // undecidable
}

std::ostream& operator<<( std::ostream& out, const Sequence& seq )
{
  for ( number_t i = 0; i < seq.size(); ++i )
  {
    if ( i > 0 ) out << ",";
    out << seq[i];
  }
  return out;
}

std::string Sequence::to_string() const
{
  std::stringstream ss;
  ss << (*this);
  return ss.str();
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
  if ( index >= size() )
  {
    resize( index + 1, 0 );
  }
  (*this)[index] = value;
}

Memory Memory::fragment( number_t start, number_t length ) const
{
  Memory f;
  for ( number_t i = 0; i < length; ++i )
  {
    if ( start + i >= size() )
    {
      break;
    }
    f.set( i, (*this)[start + i] );
  }
  return f;
}

bool Memory::operator<( const Memory& m ) const
{
  number_t length = size() > m.size() ? size() : m.size();
  for ( number_t i = 0; i < length; ++i )
  {
    if ( get( i ) < m.get( i ) )
    {
      return true; // less
    }
    else if ( get( i ) > m.get( i ) )
    {
      return false; // greater
    }
  }
  return false; // equal
}

bool Memory::operator==( const Memory& m ) const
{
  number_t length = size() > m.size() ? size() : m.size();
  for ( number_t i = 0; i < length; ++i )
  {
    if ( get( i ) != m.get( i ) )
    {
      return false;
    }
  }
  return true; // equal
}

bool Memory::operator!=( const Memory& m ) const
{
  return !(*this == m);
}

std::ostream& operator<<( std::ostream& out, const Memory& m )
{
  out << "[";
  for ( number_t i = 0; i < m.size(); ++i )
  {
    if ( i > 0 ) out << ",";
    out << m[i];
  }
  out << "]";
  return out;
}
