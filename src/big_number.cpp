#include "big_number.hpp"

bool BigNumber::parse( const std::string& s )
{
  int64_t size = s.length();
  is_negative = (s[0] == '-');
  size_t w = 0;
  while ( true )
  {
    if ( (size <= 0) || (is_negative && size <= 1) )
    {
      break;
    }
    if ( w >= BigNumber::NUM_WORDS )
    {
      return false;
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
  return true;
}

void BigNumber::print( std::ostream & out )
{
  if ( is_negative )
  {
    out << '-';
  }
  bool print = false;
  char ch;
  for ( size_t w = 0; w < BigNumber::NUM_WORDS; w++ )
  {
    const auto word = words[BigNumber::NUM_WORDS - w - 1];
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
}
