#include "number.hpp"

#include "util.hpp"

#include <sstream>
#include <unordered_set>

// === Sequence ===============================================================

Sequence Sequence::subsequence( size_t start ) const
{
  return Sequence( std::vector<number_t>( begin() + start, end() ) );
}

Sequence Sequence::subsequence( size_t start, size_t length ) const
{
  return Sequence( std::vector<number_t>( begin() + start, begin() + start + length ) );
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

number_t Sequence::min( bool includeNegative ) const
{
  number_t min = 0;
  number_t cur = 0;
  for ( size_t i = 0; i < size(); ++i )
  {
    cur = (*this)[i];
    if ( i == 0 || ((cur < min) && (includeNegative || cur >= 0)) )
    {
      min = cur;
    }
  }
  return min;
}

void Sequence::add( number_t n )
{
  for ( size_t i = 0; i < size(); ++i )
  {
    (*this)[i] += n;
  }
}

void Sequence::sub( number_t n )
{
  for ( size_t i = 0; i < size(); ++i )
  {
    if ( (*this)[i] > n )
    {
      (*this)[i] = (*this)[i] - n;
    }
    else
    {
      (*this)[i] = 0;
    }
  }
}

number_t Sequence::sum() const
{
  number_t sum = 0;
  for ( size_t x = 0; x < size(); x++ )
  {
    sum += (*this)[x];
  }
  return sum;
}

bool Sequence::align( const Sequence &s, int64_t max_offset )
{
  // check if they agree on prefix already
  size_t min_length = std::min( size(), s.size() );
  bool agree = true;
  for ( size_t i = 0; i < min_length; ++i )
  {
    if ( (*this)[i] != s[i] )
    {
      agree = false;
      break;
    }
  }
  if ( agree )
  {
    return true;
  }

  // try to align them
  max_offset = std::abs( max_offset );
  for ( int64_t offset = 1; offset <= max_offset; ++offset )
  {
    if ( offset >= (int64_t) min_length )
    {
      break;
    }
    bool agree_pos = true;
    bool agree_neg = true;
    for ( size_t i = 0; i < min_length; ++i )
    {
      if ( i + offset < size() && (*this)[i + offset] != s[i] )
      {
        agree_pos = false;
      }
      if ( i + offset < s.size() && (*this)[i] != s[i + offset] )
      {
        agree_neg = false;
      }
    }
    if ( agree_pos )
    {
      erase( begin(), begin() + offset );
      return true;
    }
    if ( agree_neg )
    {
      insert( begin(), s.begin(), s.begin() + offset );
      return true;
    }
  }
  return false;
}

bool Sequence::operator<( const Sequence &m ) const
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

bool Sequence::operator==( const Sequence &m ) const
{
  if ( size() != m.size() )
  {
    return false;
  }
  for ( size_t i = 0; i < size(); ++i )
  {
    if ( (*this)[i] != m[i] )
    {
      return false; // not equal
    }
  }
  return true;
}

bool Sequence::operator!=( const Sequence &m ) const
{
  return !((*this) == m);
}

std::ostream& operator<<( std::ostream &out, const Sequence &seq )
{
  for ( size_t i = 0; i < seq.size(); ++i )
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

void SequenceToIdsMap::remove( Sequence seq, size_t id )
{
  auto ids = find( seq );
  if ( ids != end() )
  {
    auto it = ids->second.begin();
    while ( it != ids->second.end() )
    {
      if ( *it == id )
      {
        it = ids->second.erase( it );
      }
      else
      {
        ++it;
      }
    }
  }
}

Sequence Polynomial::eval( number_t length ) const
{
  Sequence seq;
  seq.resize( length );
  for ( size_t n = 0; n < seq.size(); n++ )
  {
    for ( size_t d = 0; d < size(); d++ )
    {
      number_t f = 1;
      for ( size_t e = 0; e < d; e++ )
      {
        f *= n;
      }
      seq[n] += (*this)[d] * f;
    }
  }
  return seq;
}

std::string Polynomial::to_string() const
{
  std::stringstream s;
  bool empty = true;
  for ( int i = ((int) size()) - 1; i >= 0; i-- )
  {
    auto factor = (*this)[i];
    if ( factor != 0 )
    {
      if ( !empty ) s << "+";
      empty = false;
      s << factor;
      if ( i > 0 ) s << "n";
      if ( i > 1 ) s << "^" << i;
    }
  }
  if ( empty ) s << "0";
  return s.str();
}

Polynomial operator+( const Polynomial &a, const Polynomial &b )
{
  Polynomial c = a;
  if ( b.size() > c.size() )
  {
    c.resize( b.size() );
  }
  for ( size_t i = 0; i < c.size(); i++ )
  {
    c[i] += b[i];
  }
  return c;
}

Polynomial operator-( const Polynomial &a, const Polynomial &b )
{
  Polynomial c = a;
  if ( b.size() > c.size() )
  {
    c.resize( b.size() );
  }
  for ( size_t i = 0; i < c.size(); i++ )
  {
    if ( i < b.size() )
    {
      c[i] = c[i] - b[i];
    }
  }
  return c;
}
