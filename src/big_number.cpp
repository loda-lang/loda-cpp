#include "big_number.hpp"

#include <limits>
#include <vector>

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

int64_t BigNumber::asInt() const
{
  if ( is_infinite )
  {
    throw std::runtime_error( "Infinity error" );
  }
  for ( size_t i = 1; i < NUM_WORDS; i++ )
  {
    if ( words[i] != 0 )
    {
      throw std::runtime_error( "Integer overflow" );
    }
  }
  return is_negative ? -words[0] : words[0];
}

int64_t BigNumber::getNumUsedWords() const
{
  if ( is_infinite )
  {
    return 1;
  }
  for ( int64_t i = NUM_WORDS - 1; i >= 0; i-- )
  {
    if ( words[i] != 0 )
    {
      return i + 1;
    }
  }
  return 1;
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
  bool is_zero = true;
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
    is_zero = is_zero && (words[i] != 0);
  }
  return !is_zero && is_negative && !n.is_negative;
}

BigNumber& BigNumber::negate()
{
  // note that this can lead to -0 (therefore we don't expose is_negative)
  is_negative = !is_negative;
  return *this;
}

BigNumber& BigNumber::operator+=( const BigNumber& n )
{
  if ( is_infinite || n.is_infinite )
  {
    makeInfinite();
    return *this;
  }
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
  if ( is_infinite || n.is_infinite )
  {
    makeInfinite();
    return *this;
  }
  BigNumber result( 0 );
  int64_t shift = 0;
  const int64_t s = n.getNumUsedWords(); // <= NUM_WORDS
  for ( int64_t i = 0; i < s; i++ )
  {
    auto copy = *this;
    copy.mulShort( n.words[i] % WORD_BASE_ROOT ); // low bits
    copy.shift( shift );
    shift += 1;
    result += copy;
    copy = *this;
    copy.mulShort( n.words[i] / WORD_BASE_ROOT ); // high bits
    copy.shift( shift );
    shift += 1;
    result += copy;
    if ( result.is_infinite )
    {
      break;
    }
  }
  if ( !result.is_infinite )
  {
    result.is_negative = (is_negative != n.is_negative);
  }
  (*this) = result;
  return (*this);
}

void BigNumber::mulShort( int64_t n )
{
  int64_t carry = 0;
  const int64_t s = std::min<int64_t>( getNumUsedWords() + 1, NUM_WORDS );
  for ( int64_t i = 0; i < s; i++ )
  {
    auto& w = words[i];
    const int64_t h = n * (w / WORD_BASE_ROOT);
    const int64_t l = n * (w % WORD_BASE_ROOT);
    const int64_t t = (h % WORD_BASE_ROOT) * WORD_BASE_ROOT;
    w = l + t + carry;
    carry = h / WORD_BASE_ROOT;
  }
  if ( carry )
  {
    makeInfinite();
  }
}

void BigNumber::shift( int64_t n )
{
  for ( ; n > 0; n-- )
  {
    int64_t next = 0;
    for ( size_t i = 0; i < NUM_WORDS; i++ )
    {
      int64_t h = words[i] / WORD_BASE_ROOT;
      int64_t l = words[i] % WORD_BASE_ROOT;
      words[i] = (l * WORD_BASE_ROOT) + next;
      next = h;
    }
    if ( next )
    {
      makeInfinite();
      break;
    }
  }
}

BigNumber& BigNumber::operator/=( const BigNumber& n )
{
  if ( is_infinite || n.is_infinite || n.isZero() )
  {
    makeInfinite();
    return *this;
  }
  auto m = n; // TODO: avoid this copy is possible
  bool new_is_negative = (m.is_negative != is_negative);
  m.is_negative = false;
  is_negative = false;
  div( m );
  is_negative = new_is_negative;
  return *this;
}

void BigNumber::div( const BigNumber& n )
{
  if ( n.getNumUsedWords() == 1 && n.words[0] < WORD_BASE_ROOT )
  {
    divShort( n.words[0] );
  }
  else
  {
    divBig( n );
  }
}

void BigNumber::divShort( const int64_t n )
{
  int64_t carry = 0;
  for ( int64_t i = NUM_WORDS - 1; i >= 0; i-- )
  {
    auto& w = words[i];
    const int64_t h = w / WORD_BASE_ROOT;
    const int64_t l = w % WORD_BASE_ROOT;
    const int64_t t = (carry * WORD_BASE_ROOT) + h;
    const int64_t h2 = t / n;
    carry = t % n;
    const int64_t u = (carry * WORD_BASE_ROOT) + l;
    const int64_t l2 = u / n;
    carry = u % n;
    w = (h2 * WORD_BASE_ROOT) + l2;
  }
}

void BigNumber::divBig( const BigNumber& n )
{
  std::vector<std::pair<BigNumber, BigNumber>> d;
  BigNumber f( n );
  BigNumber g( 1 );
  while ( f < *this || f == *this )
  {
    d.push_back( std::pair<BigNumber, BigNumber>( f, g ) );
    f += f;
    g += g;
    if ( f.is_infinite || g.is_infinite )
    {
      makeInfinite();
      return;
    }
  }
  BigNumber r( 0 );
  for ( auto it = d.rbegin(); it != d.rend(); it++ )
  {
    while ( it->first < *this || it->first == *this )
    {
      sub( it->first );
      r.add( it->second );
      if ( r.is_infinite )
      {
        break;
      }
    }
  }
  *this = r;
}

BigNumber& BigNumber::operator%=( const BigNumber& n )
{
  if ( is_infinite || n.is_infinite || n.isZero() )
  {
    makeInfinite();
    return *this;
  }
  auto m = n;
  const bool new_is_negative = is_negative;
  m.is_negative = false;
  is_negative = false;
  auto q = *this;
  q.div( m );
  if ( q.is_infinite )
  {
    makeInfinite();
    return *this;
  }
  q *= m;
  if ( q.is_infinite )
  {
    makeInfinite();
    return *this;
  }
  sub( q );
  is_negative = new_is_negative;
  return *this;
}

std::size_t BigNumber::hash() const
{
  if ( is_infinite )
  {
    return std::numeric_limits<std::size_t>::max();
  }
  std::size_t seed = 0;
  bool is_zero = true;
  for ( auto &w : words )
  {
    seed ^= w + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    is_zero = is_zero && (w != 0);
  }
  if ( !is_zero && is_negative )
  {
    seed ^= 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
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
