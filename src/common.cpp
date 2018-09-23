#include "common.hpp"

Sequence::Sequence()
{
}

number_t Sequence::Get( number_t index ) const
{
  if ( index >= data.size() )
  {
    return 0;
  }
  return data[index];
}

void Sequence::Set( number_t index, number_t value )
{
/*  if ( index > 10000 )
  {
    throw std::runtime_error( "sequence too long: " + std::to_string( index ) );
  }
*/  if ( index >= data.size() )
  {
    data.resize( index + 1, 0 );
  }
  data[index] = value;
}

number_t Sequence::Length() const
{
  return data.size();
}

bool Sequence::operator<( const Sequence& other ) const
{
  number_t length = Length() > other.Length() ? Length() : other.Length();
  for ( number_t i = 0; i < length; i++ )
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

bool Sequence::operator==( const Sequence& other ) const
{
  number_t length = Length() > other.Length() ? Length() : other.Length();
  for ( number_t i = 0; i < length; i++ )
  {
    if ( Get( i ) != other.Get( i ) )
    {
      return false;
    }
  }
  return true; // equal
}

bool Sequence::operator!=( const Sequence& other ) const
{
  return !(*this == other);
}

Sequence Sequence::Fragment( number_t start, number_t length ) const
{
  Sequence f;
  for ( number_t i = 0; i < length; i++ )
  {
    f.Set( i, Get( start + i ) );
  }
  return f;
}

std::ostream& operator<<( std::ostream& out, const Sequence& seq )
{
  for ( size_t i = 0; i < seq.data.size(); i++ )
  {
    if ( i > 0 ) out << ",";
    out << seq.data[i];
  }
  return out;
}
