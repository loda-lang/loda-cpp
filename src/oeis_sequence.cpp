#include "oeis_sequence.hpp"

#include "parser.hpp"
#include "util.hpp"

#include <iomanip>
#include <fstream>
#include <sstream>

const size_t OeisSequence::LONG_SEQ_LENGTH = 250;

const size_t OeisSequence::VERY_LONG_SEQ_LENGTH = 2000;

std::string OeisSequence::getHome()
{
  // don't remove the trailing /
  return getLodaHome() + "oeis/";
}

OeisSequence::OeisSequence( size_t id )
    : id( id ),
      attempted_bfile( false ),
      loaded_bfile( false )
{
}

OeisSequence::OeisSequence( std::string id_str )
    : attempted_bfile( false ),
      loaded_bfile( false )
{
  if ( id_str.empty() || id_str[0] != 'A' )
  {
    throw std::invalid_argument( id_str );
  }
  id_str = id_str.substr( 1 );
  for ( char c : id_str )
  {
    if ( !std::isdigit( c ) )
    {
      throw std::invalid_argument( "A" + id_str );
    }
  }
  id = std::stoll( id_str );
}

OeisSequence::OeisSequence( size_t id, const std::string &name, const Sequence &full )
    : id( id ),
      name( name ),
      full( full ),
      attempted_bfile( false ),
      loaded_bfile( false )
{
}

std::ostream& operator<<( std::ostream &out, const OeisSequence &s )
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

std::string OeisSequence::id_str( const std::string &prefix ) const
{
  std::stringstream s;
  s << prefix << std::setw( 6 ) << std::setfill( '0' ) << id;
  return s.str();
}

std::string OeisSequence::dir_str() const
{
  std::stringstream s;
  s << std::setw( 3 ) << std::setfill( '0' ) << (id / 1000);
  return s.str();
}

std::string OeisSequence::url_str() const
{
  return "https://oeis.org/" + id_str();
}

std::string OeisSequence::getProgramPath() const
{
  return "programs/oeis/" + dir_str() + "/" + id_str() + ".asm";
}

std::string OeisSequence::getBFilePath() const
{
  return getHome() + "b/" + dir_str() + "/" + id_str( "b" ) + ".txt";
}

bool loadBFile( size_t id, const Sequence& seq_full, Sequence& seq_big, size_t num_terms )
{
  const OeisSequence oeis_seq( id );
  bool has_b_file = false;
  bool has_overflow = false;

  // try to read b-file
  std::ifstream big_file( oeis_seq.getBFilePath() );
  if ( big_file.good() )
  {
    has_b_file = true;
    std::string l;
    int64_t expected_index = -1, index = 0, value = 0;
    while ( std::getline( big_file, l ) )
    {
      l.erase( l.begin(), std::find_if( l.begin(), l.end(), []( int ch )
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
        Log::get().warn( "Unexpected index " + std::to_string( index ) + " in b-file " + oeis_seq.getBFilePath() );
        return false;
      }
      if ( !ss )
      {
        has_overflow = true;
        break;
      }
      if ( isCloseToOverflow( value ) )
      {
        has_overflow = true;
        break;
      }
      seq_big.push_back( value );
      ++expected_index;
    }
    if ( Log::get().level == Log::Level::DEBUG )
    {
      Log::get().debug(
          "Read b-file for " + oeis_seq.id_str() + " with " + std::to_string( seq_big.size() ) + " terms" );
    }
  }

  // not found?
  if ( seq_big.empty() && !has_b_file )
  {
    if ( Log::get().level == Log::Level::DEBUG )
    {
      Log::get().debug( "b-file not found: " + oeis_seq.id_str() );
    }
    return false;
  }

  // align sequences on common prefix (will verify correctness below again!)
  seq_big.align( seq_full, 5 );

  // check length
  std::string error_state;

  if ( seq_big.size() < seq_full.size() )
  {
    // big should never be shorter (there can be parser issues causing this)
    seq_big = seq_full;
  }

  if ( seq_big.empty() )
  {
    error_state = "empty";
  }
  else
  {
    // check that the sequences agree on prefix
    Sequence seq_test( std::vector<number_t>( seq_big.begin(), seq_big.begin() + seq_full.size() ) );
    if ( seq_test != seq_full )
    {
      Log::get().warn( "Unexpected terms in b-file or program for " + oeis_seq.id_str() );
      Log::get().warn( "- expected: " + seq_full.to_string() );
      Log::get().warn( "- found:    " + seq_test.to_string() );
      error_state = "invalid";
    }
  }

  // remove b-files if they are issues (we use a heuristic to avoid massive amount of downloads at the same time)
  if ( !error_state.empty() )
  {
    // TODO: also refetch old files, see getFileAgeInDays( oeis_seq.getBFilePath() )
    Log::get().warn( "Removing " + error_state + " b-file " + oeis_seq.getBFilePath() );
    std::remove( oeis_seq.getBFilePath().c_str() );
    return false;
  }

  // shrink big sequence to maximum number of terms
  if ( seq_big.size() > num_terms )
  {
    seq_big = Sequence( std::vector<number_t>( seq_big.begin(), seq_big.begin() + num_terms ) );
  }

  if ( Log::get().level == Log::Level::DEBUG )
  {
    Log::get().debug(
        "Loaded long version of sequence " + oeis_seq.id_str() + " with " + std::to_string( seq_big.size() )
            + " terms" );
  }
  return true;
}

Sequence OeisSequence::getTerms( int64_t max_num_terms ) const
{
  size_t real_max_terms = (max_num_terms >= 0) ? max_num_terms : VERY_LONG_SEQ_LENGTH;
  if ( real_max_terms > full.size() && !attempted_bfile )
  {
    attempted_bfile = true;
    if ( id != 0 )
    {
      Sequence big;
      if ( loadBFile( id, full, big, real_max_terms ) )
      {
        // use data from big sequence from now on
        loaded_bfile = true;
        full = big;
      }
    }
  }
  real_max_terms = std::min( real_max_terms, full.size() );
  if ( real_max_terms == full.size() )
  {
    return full;
  }
  else
  {
    return Sequence( std::vector<number_t>( full.begin(), full.begin() + real_max_terms ) );
  }
}

void OeisSequence::fetchBFile( int64_t max_num_terms ) const
{
  if ( !attempted_bfile )
  {
    getTerms( max_num_terms );
  }
  if ( !loaded_bfile )
  {
    std::ifstream big_file( getBFilePath() );
    if ( !big_file.good() )
    {
      ensureDir( getBFilePath() );
      std::string cmd = "wget -nv --no-use-server-timestamps -O " + getBFilePath() + " " + url_str() + "/"
          + id_str( "b" ) + ".txt";
      if ( system( cmd.c_str() ) != 0 )
      {
        Log::get().error( "Error fetching b-file for " + id_str(), true ); // need to exit here to be able to cancel
      }
      attempted_bfile = false; // reload on next access
    }
  }
}
