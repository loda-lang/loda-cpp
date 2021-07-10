#include "big_number.hpp"

BigNumber::BigNumber()
    : is_negative( false ),
      is_infinite( false )
{
  words.fill( 0 );
}

BigNumber::BigNumber( int64_t value )
{
  if ( value >= 0 && static_cast<uint64_t>( value ) < WORD_BASE )
  {
    is_negative = false;
    is_infinite = false;
    words.fill( 0 );
    words[0] = value;
  }
  else
  {
    load( std::to_string( value ) );
  }
}

BigNumber::BigNumber( const std::string& s )
{
  load( s );
}

void throwNumberParseError( const std::string& s )
{
  throw std::invalid_argument( "error reading number: '" + s + "'" );
}

void BigNumber::load( const std::string& s )
{
  if ( s == "inf" )
  {
    makeInfinite();
    return;
  }
  is_infinite = false;
  int64_t size = s.length();
  int64_t start = 0;
  while ( start < size && s[start] == ' ' )
  {
    start++;
  }
  if ( start == size )
  {
    throwNumberParseError( s );
  }
  if ( s[start] == '-' )
  {
    is_negative = true;
    if ( ++start == size )
    {
      throwNumberParseError( s );
    }
  }
  else
  {
    is_negative = false;
  }
  size -= start;
  while ( size > 0 && s[start + size - 1] == ' ' )
  {
    size--;
  }
  if ( size == 0 )
  {
    throwNumberParseError( s );
  }
  size_t w = 0;
  while ( size > 0 )
  {
    if ( w >= BigNumber::NUM_WORDS )
    {
      makeInfinite();
      return;
    }
    int64_t length = 0;
    int64_t num = 0;
    int64_t prefix = 1;
    char ch;
    for ( int64_t i = size - 1; i >= 0 && i >= size - static_cast<int64_t>( BigNumber::NUM_WORD_DIGITS ); --i )
    {
      ch = s[start + i];
      if ( ch < '0' || ch > '9' )
      {
        throwNumberParseError( s );
      }
      num += (ch - '0') * prefix;
      prefix *= 10;
      ++length;
    }
    words[w++] = num;
    size -= length;
  }
  while ( w < BigNumber::NUM_WORDS )
  {
    words[w++] = 0;
  }
}

bool BigNumber::isZero() const
{
  if ( is_infinite )
  {
    return false;
  }
  for ( auto& word : words )
  {
    if ( word != 0 )
    {
      return false;
    }
  }
  return true;
}

void BigNumber::makeInfinite()
{
  is_negative = false;
  is_infinite = true;
  words.fill( 0 );
}

BigNumber BigNumber::minMax( bool is_max )
{
  BigNumber m;
  m.is_infinite = false;
  m.is_negative = !is_max;
  for ( auto& word : m.words )
  {
    word = WORD_BASE - 1;
  }
  return m;
}

bool BigNumber::operator==( const BigNumber&n ) const
{
  if ( is_infinite != n.is_infinite )
  {
    return false;
  }
  if ( words != n.words )
  {
    return false;
  }
  return ((is_negative == n.is_negative) || isZero());
}

bool BigNumber::operator!=( const BigNumber&n ) const
{
  return !(*this == n);
}

bool BigNumber::operator<( const BigNumber&n ) const
{
  for ( int64_t i = NUM_WORDS - 1; i >= 0; i-- )
  {
    if ( words[i] < n.words[i] )
    {
      return !n.is_negative;
    }
    else if ( words[i] > n.words[i] )
    {
      return is_negative;
    }
  }
  return false;
}

BigNumber& BigNumber::negate()
{
  // note that this can lead to -0 (therefore we don't expose is_negative)
  is_negative = !is_negative;
  return *this;
}

BigNumber& BigNumber::operator+=( const BigNumber& n )
{
  // check if one of the operands is negative
  if ( !is_negative && n.is_negative )
  {
    BigNumber m( n );
    m.is_negative = false;
    if ( *this < m )
    {
      m.sub( *this );
      (*this) = m;
      is_negative = true;
    }
    else
    {
      sub( m );
    }
  }
  else if ( is_negative && !n.is_negative )
  {
    BigNumber m( n );
    is_negative = false;
    if ( *this < m )
    {
      m.sub( *this );
      (*this) = m;
    }
    else
    {
      sub( m );
      is_negative = true;
    }
  }
  else
  {
    add( n );
  }
  return *this;
}

void BigNumber::add( const BigNumber& n )
{
  auto it1 = words.begin();
  auto it2 = n.words.begin();
  int64_t sum = 0;
  while ( it1 != words.end() || it2 != n.words.end() )
  {
    if ( it1 != words.end() )
    {
      sum += *it1;
    }
    else
    {
      makeInfinite();
      return;
    }
    if ( it2 != n.words.end() )
    {
      sum += *it2;
      ++it2;
    }
    *it1 = sum % WORD_BASE;
    ++it1;
    sum /= WORD_BASE;
  }
  if ( sum )
  {
    makeInfinite();
  }
}

void BigNumber::sub( const BigNumber& n )
{
  auto it1 = words.begin();
  auto it2 = n.words.begin();
  int64_t d = 0;
  while ( it1 != words.end() || it2 != n.words.end() )
  {
    if ( it1 != words.end() )
    {
      d += *it1;
      ++it1;
    }
    if ( it2 != n.words.end() )
    {
      d -= *it2;
      ++it2;
    }
    if ( d < 0 )
    {
      *(it1 - 1) = d + WORD_BASE;
      d = -1;
    }
    else
    {
      *(it1 - 1) = d % WORD_BASE;
      d /= WORD_BASE;
    }
  }
  if ( d < 0 )
  {
    is_negative = true;
  }
}

BigNumber& BigNumber::operator*=( const BigNumber& n )
{
  throw std::runtime_error( "Bigint not supported for *=" );
}

BigNumber& BigNumber::operator/=( const BigNumber& n )
{
  throw std::runtime_error( "Bigint not supported for /=" );
}

BigNumber& BigNumber::operator%=( const BigNumber& n )
{
  throw std::runtime_error( "Bigint not supported for %=" );
}

std::size_t BigNumber::hash() const
{
  throw std::runtime_error( "Bigint not supported for hash" );
}

std::ostream& operator<<( std::ostream &out, const BigNumber &n )
{
  if ( n.is_infinite )
  {
    out << "inf";
    return out;
  }
  if ( n.is_negative && !n.isZero() )
  {
    out << '-';
  }
  bool print = false;
  char ch;
  for ( size_t w = 0; w < BigNumber::NUM_WORDS; w++ )
  {
    const auto word = n.words[BigNumber::NUM_WORDS - w - 1];
    auto base = BigNumber::WORD_BASE / 10;
    while ( base )
    {
      ch = static_cast<char>( '0' + ((word / base) % 10) );
      print = print || (ch != '0');
      if ( print )
      {
        out << ch;
      }
      base /= 10;
    }
  }
  if ( !print )
  {
    out << '0';
  }
  return out;
}
