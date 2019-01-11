#include "oeis.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "printer.hpp"
#include "util.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>

#define MAX_TERMS 200

std::ostream& operator<<( std::ostream& out, const OeisSequence& s )
{
  out << s.id_str() << ": " << s.name;
  return out;
}

std::string OeisSequence::to_string() const
{
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

std::string OeisSequence::id_str( const std::string& prefix ) const
{
  std::stringstream s;
  s << prefix << std::setw( 6 ) << std::setfill( '0' ) << id;
  return s.str();
}

void throwParseError( const std::string& line )
{
  Log::get().error( "error parsing oeis line: " + line, true );
}

Oeis::Oeis( const Settings& settings )
    : settings( settings ),
      total_count_( 0 )
{
}

void Oeis::load()
{
  // load sequence data
  Log::get().debug( "Loading sequence data from the OEIS" );
  std::string home = std::string( std::getenv( "HOME" ) ) + "/.loda/oeis/";
  std::ifstream stripped( home + "stripped" );
  if ( !stripped.good() )
  {
    Log::get().error( "OEIS data not found: run get_oeis.sh to download it", true );
  }
  std::string line;
  size_t pos;
  number_t id, num;
  Sequence full_sequence;
  Sequence norm_sequence;
  Sequence big_sequence;
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
    ++total_count_;
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
        full_sequence.clear();
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

    // normalized sequence
    norm_sequence = Sequence(
        std::vector<number_t>( full_sequence.begin(), full_sequence.begin() + settings.num_terms ) );

    // big sequence
    big_sequence.clear();
    std::stringstream big_path;
    big_path << home << "b/b" << std::setw( 6 ) << std::setfill( '0' ) << id << ".txt";
    std::ifstream big_file( big_path.str() );
    if ( big_file.good() )
    {
      std::string l;
      int64_t expected_index = -1, index = 0, value = 0;
      while ( std::getline( big_file, l ) )
      {
        l.erase( l.begin(), std::find_if( l.begin(), l.end(), [](int ch)
        {
          return !std::isspace(ch);
        } ) );
        if ( l.empty() || l[0] == '#' )
        {
          continue;
        }
        std::stringstream ss( l );
        ss >> index >> value;
        if ( expected_index == -1 )
        {
          expected_index = index;
        }
        if ( index != expected_index )
        {
          Log::get().warn( "Unexpected index " + std::to_string( index ) + " in b-file " + big_path.str() );
          big_sequence.clear();
          break;
        }
        if ( value < 0 )
        {
          big_sequence.clear();
          break;
        }
        if ( value == std::numeric_limits<int64_t>::max() )
        {
          break;
        }
        big_sequence.push_back( value );
        ++expected_index;
      }
      if ( big_sequence.size() >= full_sequence.size() )
      {
        Sequence test_sequence(
            std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + full_sequence.size() ) );
        if ( test_sequence != full_sequence )
        {
          Log::get().warn( "Unexpected terms in b-file " + big_path.str() );
          big_sequence.clear();
        }
      }
      if ( big_sequence.size() > MAX_TERMS )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + MAX_TERMS ) );
      }
      if ( big_sequence.size() == 64 )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + 63 ) );
      }
      if ( id == 94966 )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + big_sequence.size() - 2 ) );
      }
      if ( !big_sequence.empty() )
      {
        full_sequence = big_sequence;
        Log::get().debug(
            "Loaded b-file for sequence " + std::to_string( id ) + " with " + std::to_string( big_sequence.size() )
                + " terms" );
      }
    }
    else
    {
      Log::get().debug( "b-file not found: " + big_path.str() );
    }

    // add sequence to index
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
  std::ifstream names( home + "names" );
  if ( !names.good() )
  {
    Log::get().error( "OEIS data not found: run get_oeis.sh to download it", true );
  }
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
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count_ ) + " sequences from the OEIS" );
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

void Oeis::dumpProgram( number_t id, Program p, const std::string& file ) const
{
  p.removeOps( Operation::Type::NOP );
  std::ofstream out( file );
  auto& seq = sequences.at( id );
  out << "; " << seq << std::endl;
  out << "; " << seq.full << std::endl << std::endl;
  Printer r;
  r.print( p, out );
}
