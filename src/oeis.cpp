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

size_t Oeis::MAX_NUM_TERMS = 250;

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
  Log::get().error( "error parsing OEIS line: " + line, true );
}

std::string getHome()
{
  return std::string( std::getenv( "HOME" ) ) + "/.loda/oeis/";
}

Oeis::Oeis( const Settings& settings )
    : settings( settings ),
      interpreter( settings ),
      total_count_( 0 ),
      search_linear_( false )
{
}

void Oeis::load()
{
  // load sequence data
  Log::get().debug( "Loading sequence data from the OEIS" );
  std::ifstream stripped( getHome() + "stripped" );
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
    if ( pos >= line.length() || line[pos] != ' ' || id == 0 )
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

    // check minimum number of terms
    if ( full_sequence.size() < settings.num_terms )
    {
      continue;
    }

    // normalized sequence
    norm_sequence = Sequence(
        std::vector<number_t>( full_sequence.begin(), full_sequence.begin() + settings.num_terms ) );

    // big sequence
    big_sequence.clear();
    std::stringstream big_path;
    big_path << getHome() << "b/b" << std::setw( 6 ) << std::setfill( '0' ) << id << ".txt";
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
          if ( index < 0 )
          {
            continue;
          }
          expected_index = index;
          if ( value != (int) full_sequence.front() )
          {
            for ( size_t i = 0; i < 5; i++ )
            {
              if ( value != (int) full_sequence[i] )
              {
                big_sequence.push_back( full_sequence[i] );
              }
              else
              {
                break;
              }
            }
          }
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
      if ( big_sequence.size() > MAX_NUM_TERMS )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + MAX_NUM_TERMS ) );
      }
      if ( big_sequence.size() == 64 )
      {
        big_sequence = Sequence( std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + 63 ) );
      }
      if ( id == 94966 )
      {
        big_sequence = Sequence(
            std::vector<number_t>( big_sequence.begin(), big_sequence.begin() + big_sequence.size() - 2 ) );
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
    ids[norm_sequence].push_back( id );

    number_t offset = norm_sequence.min();
    if ( offset > 0 )
    {
      auto zero_offset_sequence = norm_sequence;
      zero_offset_sequence.sub( offset );
      ids_zero_offset[zero_offset_sequence].push_back( id );
    }

    ++loaded_count;
  }

  loadNames();

  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count_ ) + " sequences from the OEIS" );

}

void Oeis::loadNames()
{
  Log::get().debug( "Loading sequence names from the OEIS" );
  std::ifstream names( getHome() + "names" );
  if ( !names.good() )
  {
    Log::get().error( "OEIS data not found: run get_oeis.sh to download it", true );
  }
  std::string line;
  size_t pos;
  number_t id;
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
    if ( pos >= line.length() || line[pos] != ' ' || id == 0 )
    {
      throwParseError( line );
    }
    ++pos;
    if ( id < sequences.size() && sequences[id].id == id )
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
}

const std::vector<OeisSequence>& Oeis::getSequences() const
{
  return sequences;
}

std::vector<number_t> Oeis::findSequence( const Program& p ) const
{
  std::vector<number_t> result;
  Sequence norm_seq;
  try
  {
    norm_seq = interpreter.eval( p );
  }
  catch ( const std::exception& )
  {
    return result;
  }
  if ( !search_linear_ && norm_seq.is_linear() )
  {
    return result;
  }
  findDirect( p, norm_seq, result );
  return result;
}

void Oeis::findDirect( const Program& p, const Sequence& norm_seq, std::vector<number_t>& result ) const
{
  auto it = ids.find( norm_seq );
  if ( it == ids.end() )
  {
    return;
  }
  Sequence full_seq;
  for ( auto id : it->second )
  {
    auto& expected_full_seq = sequences.at( id ).full;
    try
    {
      if ( full_seq.size() != expected_full_seq.size() )
      {
        full_seq = interpreter.eval( p, expected_full_seq.size() );
      }
      if ( full_seq.size() == expected_full_seq.size() && !(full_seq != expected_full_seq) )
      {
        result.push_back( id );
      }
    }
    catch ( const std::exception& )
    {
      // program not okay
    }
  }
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
