#include "oeis.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "util.hpp"

#include <iomanip>
#include <fstream>
#include <sstream>

std::ostream& operator<<( std::ostream& out, const OeisSequence& s )
{
  out << s.id_str() << ": " << s.name;
  return out;
}

std::string OeisSequence::id_str() const
{
  std::stringstream s;
  s << "A" << std::setw( 6 ) << std::setfill( '0' ) << id;
  return s.str();
}

void throwParseError( const std::string& line )
{
  Log::get().error( "error parsing oeis line: " + line, true );
}

Oeis::Oeis( const Settings& settings )
    : settings( settings )
{
}

void Oeis::load()
{
  // load sequence data
  Log::get().debug( "Loading sequence data from the OEIS" );
  std::ifstream stripped( "stripped" );
  std::string line;
  int pos;
  number_t id, num;
  Sequence full_sequence;
  Sequence norm_sequence;
  size_t total_count = 0;
  size_t loaded_count = 0;
  while ( std::getline( stripped, line ) )
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
    full_sequence.clear();
    while ( pos < line.length() )
    {
      if ( line[pos] == ',' )
      {
        full_sequence.push_back( num );
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
    if ( full_sequence.size() < settings.num_terms || full_sequence.distinct_values() < 4 )
    {
      continue;
    }
    norm_sequence = Sequence(
        std::vector<number_t>( full_sequence.begin(), full_sequence.begin() + settings.num_terms ) );

    if ( id >= sequences.size() )
    {
      sequences.resize( 2 * id );
    }
    sequences[id] = OeisSequence( id, "", norm_sequence, full_sequence );
    ids[norm_sequence] = id;
    ++loaded_count;
  }

  // load sequence names
  Log::get().debug( "Loading sequence names from the OEIS" );
  std::ifstream names( "names" );
  while ( std::getline( names, line ) )
  {
    if ( line.empty() || line[0] == '#' )
    {
      continue;
    }
    if ( line[0] != 'A' )
    {
      throwParseError( line );
    }
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
    if ( id < sequences.size() )
    {
      sequences[id].name = line.substr( pos );
      if ( Log::get().level == Log::Level::DEBUG )
      {
        std::stringstream buf;
        buf << "Loaded sequence " << sequences[id];
        Log::get().debug( buf.str() );
      }
    }
  }

  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count ) + " sequences from the OEIS" );
}

number_t Oeis::score( const Sequence& s )
{
  auto it = ids.find( s );
  if ( it != ids.end() )
  {
    number_t id = it->second;
    std::stringstream buf;
    buf << "Found program for " << sequences[id];
    Log::get().info( buf.str() );
    buf.str( "" );
    buf << "First " << sequences[id].size() << " terms of " << sequences[id].id_str() << ": "
        << static_cast<Sequence>( sequences[id] );
    Log::get().debug( buf.str() );
    return 0;
  }
  return 1;
}

number_t Oeis::findSequence( const Program& p ) const
{
  Interpreter interpreter( settings );
  try
  {
    auto norm_seq = interpreter.eval( p );
    auto it = ids.find( norm_seq );
    if ( it != ids.end() )
    {
      auto expected_full_seq = sequences.at( it->second ).full;
      auto full_seq = interpreter.eval( p, expected_full_seq.size() );
      if ( full_seq.size() == expected_full_seq.size() && !(full_seq != expected_full_seq) )
      {
        return it->second;
      }
    }
  }
  catch ( const std::exception& )
  {
    // will return 0 (not found)
  }
  return 0; // not found
}
