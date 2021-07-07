#include "big_number.hpp"

BigNumber::BigNumber()
    : is_negative( false ),
      is_infinite( false )
{
  words.fill( 0 );
}

BigNumber::BigNumber( int64_t value )
{
  if ( value >= 0 && value < WORD_BASE )
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

void BigNumber::load( const std::string& s )
{
  if ( s == "inf" )
  {
    is_negative = false;
    is_infinite = true;
    words.fill( 0 );
    return;
  }
  int64_t size = s.length();
  is_negative = (s[0] == '-');
  is_infinite = false;
  size_t w = 0;
  while ( true )
  {
    if ( (size <= 0) || (is_negative && size <= 1) )
    {
      break;
    }
    if ( w >= BigNumber::NUM_WORDS )
    {
      is_infinite = true;
      words.fill( 0 );
      return;
    }
    int64_t length = 0;
    uint64_t num = 0;
    uint64_t prefix = 1;
    for ( int64_t i = size - 1; i >= 0 && i >= size - static_cast<int64_t>( BigNumber::NUM_WORD_DIGITS ); --i )
    {
      if ( s[i] < '0' || s[i] > '9' )
      {
        break;
      }
      num += (s[i] - '0') * prefix;
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
  throw std::runtime_error( "Bigint not supported for <" );
}

BigNumber& BigNumber::negate()
{
  is_negative = !is_negative;
  return *this;
}

BigNumber& BigNumber::operator+=( const BigNumber& n )
{
  throw std::runtime_error( "Bigint not supported for +=" );
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
