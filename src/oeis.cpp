#include "oeis.hpp"

#include "number.hpp"
#include "util.hpp"

#include <fstream>
#include <sstream>

void throwParseError( const std::string& line )
{
  Log::get().error( "error parsing oeis line: " + line, true );
}

void Oeis::load()
{
  std::ifstream in( "stripped" );
  std::string line;
  int pos;
  number_t id, num;
  Sequence s;
  size_t total_count = 0;
  size_t loaded_count = 0;
  while ( std::getline( in, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      throwParseError( line );
    }
    ++total_count;
    pos = 1;
    id = 0;
    for ( pos = 1; pos < line.length() && line[pos] >= '0' && line[pos] <= '9'; ++pos )
    {
      id = (10 * id) + (line[pos] - '0');
    }
    if ( pos >= line.length() || line[pos] != ' ' )
    {
      throwParseError( line );
    }
    ++pos;
    if ( pos >= line.length() || line[pos] != ',' )
    {
      throwParseError( line );
    }
    ++pos;
    num = 0;
    s.clear();
    while ( pos < line.length() )
    {
      if ( line[pos] == ',' )
      {
        s.push_back( num );
        if ( s.size() == length )
        {
          break;
        }
        num = 0;
      }
      else if ( line[pos] >= '0' && line[pos] <= '9' )
      {
        if ( num > 1844674407370955100ul )
        {
          break;
        }
        num = (10 * num) + (line[pos] - '0');
      }
      else if ( line[pos] == '-' )
      {
        break;
      }
      else
      {
        throwParseError( line );
      }
      ++pos;
    }
    if ( s.size() != length || s.subsequence( 4 ).linear() || s.distinct_values() <= 2 )
    {
      continue;
    }

//    std::cout << line << std::endl;
//    std::cout << id << ": " << s << std::endl << std::endl;

    if ( id >= sequences.size() )
    {
      sequences.resize( 2 * id );
    }
    sequences[id] = OeisSequence( id, "", s );
    ids[s] = id;
    ++loaded_count;
  }
  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count ) + " OEIS sequences" );
}

number_t Oeis::score( const Sequence& s )
{
  auto it = ids.find( s );
  if ( it != ids.end() )
  {
    number_t id = it->second;
    std::stringstream buf;
    buf << "Found program for OEIS sequence " << id << ": " << sequences[id];
    Log::get().info( buf.str() );
    return 0;
  }
  return 1;
}
