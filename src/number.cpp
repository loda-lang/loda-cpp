#include "number.hpp"

#include "big_number.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

const Number Number::ZERO( 0 );
const Number Number::ONE( 1 );
const Number Number::INF = Number::infinity();

constexpr int64_t MIN_INT = std::numeric_limits<int64_t>::min();
constexpr int64_t MAX_INT = std::numeric_limits<int64_t>::max();
constexpr size_t MAX_SIZE = std::numeric_limits<size_t>::max();

BigNumber* INF_PTR = reinterpret_cast<BigNumber*>( 1 );

#define CONVERT_TO_BIG 0

Number::Number()
    : value( 0 ),
      big( nullptr )
{
}

Number::Number( const Number& n )
    : value( n.value ),
      big( (n.big && n.big != INF_PTR) ? new BigNumber( *n.big ) : n.big )
{
}

Number::Number( int64_t value, bool is_big )
    : value( is_big ? 0 : value ),
      big( is_big ? new BigNumber( value ) : nullptr )
{
}

Number::Number( const std::string& s, bool is_big )
{
  if ( !is_big )
  {
    big = nullptr;
    bool is_inf = false;
    try
    {
      value = std::stoll( s );
    }
    catch ( const std::out_of_range& )
    {
      is_inf = true;
    }
    if ( is_inf )
    {
      value = 0;
      big = INF_PTR;
    }
  }
  if ( is_big )
  {
    value = 0;
    big = new BigNumber( s );
    checkInfBig();
  }
}

Number::~Number()
{
  if ( big && big != INF_PTR )
  {
    delete big;
  }
}

bool Number::operator==( const Number&n ) const
{
  if ( big == INF_PTR )
  {
    return (n.big == INF_PTR);
  }
  if ( n.big == INF_PTR )
  {
    return false;
  }
  if ( big )
  {
    if ( n.big )
    {
      return (*big) == (*n.big);
    }
    else
    {
      return (*big) == BigNumber( n.value );
    }
  }
  else if ( n.big )
  {
    return BigNumber( value ) == (*n.big);
  }
  else
  {
    return (value == n.value);
  }
}

bool Number::operator!=( const Number&n ) const
{
  return !(*this == n);
}

bool Number::operator<( const Number&n ) const
{
  if ( n.big == INF_PTR )
  {
    return (big != INF_PTR);
  }
  if ( big == INF_PTR )
  {
    return false;
  }
  if ( big )
  {
    if ( n.big )
    {
      return (*big) < (*n.big);
    }
    else
    {
      return (*big) < BigNumber( n.value );
    }
  }
  else if ( n.big )
  {
    return BigNumber( value ) < (*n.big);
  }
  else
  {
    return (value < n.value);
  }
}

Number& Number::negate()
{
  if ( big == INF_PTR )
  {
    return *this;
  }
  if ( big )
  {
    big->negate();
    checkInfBig();
  }
  else if ( value == MIN_INT )
  {
    if ( CONVERT_TO_BIG )
    {
      value = 0;
      big = new BigNumber( value );
      big->negate();
      checkInfBig();
    }
    else
    {
      value = 0;
      big = INF_PTR;
    }
  }
  else
  {
    value = -value;
  }
  return *this;
}

Number& Number::operator+=( const Number& n )
{
  if ( checkInfArgs( n ) )
  {
    return *this;
  }
  if ( big )
  {
    if ( n.big )
    {
      (*big) += (*n.big);
      checkInfBig();
    }
    else
    {
      (*big) += BigNumber( n.value );
      checkInfBig();
    }
  }
  else if ( n.big )
  {
    if ( CONVERT_TO_BIG )
    {
      value = 0;
      big = new BigNumber( value );
      (*big) += (*n.big);
      checkInfBig();
    }
    else
    {
      value = 0;
      big = INF_PTR;
    }
  }
  else if ( (value > 0 && n.value > MAX_INT - value) || (value < 0 && n.value < MIN_INT - value) )
  {
    if ( CONVERT_TO_BIG )
    {
      value = 0;
      big = new BigNumber( value );
      (*big) += BigNumber( n.value );
      checkInfBig();
    }
    else
    {
      value = 0;
      big = INF_PTR;
    }
  }
  else
  {
    value += n.value;
  }
  return *this;
}

Number& Number::operator*=( const Number& n )
{
  if ( checkInfArgs( n ) )
  {
    return *this;
  }
  if ( big || n.big )
  {
    throw std::runtime_error( "Bigint not supported for *=" );
  }
  else if ( n.value != 0 && (MAX_INT / std::abs( n.value ) < std::abs( value )) )
  {
    value = 0;
    big = INF_PTR;
  }
  else
  {
    value *= n.value;
  }
  return *this;
}

Number& Number::operator/=( const Number& n )
{
  if ( checkInfArgs( n ) )
  {
    return *this;
  }
  if ( big || n.big )
  {
    throw std::runtime_error( "Bigint not supported for /=" );
  }
  if ( n.value == 0 )
  {
    value = 0;
    big = INF_PTR;
  }
  else
  {
    value /= n.value;
  }
  return *this;
}

Number& Number::operator%=( const Number& n )
{
  if ( checkInfArgs( n ) )
  {
    return *this;
  }
  if ( big || n.big )
  {
    throw std::runtime_error( "Bigint not supported for %=" );
  }
  if ( n.value == 0 )
  {
    value = 0;
    big = INF_PTR;
  }
  else
  {
    value %= n.value;
  }
  return *this;
}

int64_t Number::asInt() const
{
  if ( big == INF_PTR )
  {
    throw std::runtime_error( "Infinity error" );
  }
  if ( big )
  {
    throw std::runtime_error( "Bigint not supported for asInt" );
  }
  // TODO: throw an exception if the value is out of range
  return value;
}

std::size_t Number::hash() const
{
  if ( big == INF_PTR )
  {
    return MAX_SIZE;
  }
  if ( big )
  {
    return big->hash();
  }
  return value;
}

std::ostream& operator<<( std::ostream &out, const Number &n )
{
  if ( n.big == INF_PTR )
  {
    out << "inf";
  }
  else if ( n.big )
  {
    out << *n.big;
  }
  else
  {
    out << n.value;
  }
  return out;
}

std::string Number::to_string() const
{
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void throwParseError( std::istream& in )
{
  std::string tmp;
  std::getline( in, tmp );
  throw std::runtime_error( "Error parsing number: '" + tmp + "'" );
}

void Number::readIntString( std::istream& in, std::string& out )
{
  out.clear();
  auto ch = in.peek();
  if ( !std::isdigit( ch ) && ch != '-' )
  {
    throwParseError( in );
  }
  out += (char) ch;
  in.get();
  while ( true )
  {
    auto ch = in.peek();
    if ( !std::isdigit( ch ) )
    {
      break;
    }
    out += (char) ch;
    in.get();
  }
}

Number Number::infinity()
{
  Number inf( 0 );
  inf.big = INF_PTR;
  return inf;
}

bool Number::checkInfArgs( const Number& n )
{
  if ( big == INF_PTR )
  {
    return true;
  }
  if ( n.big == INF_PTR )
  {
    if ( big )
    {
      delete big;
    }
    big = INF_PTR;
    return true;
  }
  return false;
}

void Number::checkInfBig()
{
  if ( big && big != INF_PTR && big->isInfinite() )
  {
    delete big;
    value = 0;
    big = INF_PTR;
  }
}
