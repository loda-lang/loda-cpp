#include "oeis_sequence.hpp"

#include "util.hpp"

#include <fstream>
#include <sstream>

// TODO: this should be configurable
const size_t OeisSequence::MAX_NUM_TERMS = 250;

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

OeisSequence::OeisSequence( size_t id, const std::string &name, const Sequence &s, const Sequence &full )
    : id( id ),
      name( name ),
      norm( s ),
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

std::string OeisSequence::getProgramPath() const
{
  return "programs/oeis/" + dir_str() + "/" + id_str() + ".asm";
}

std::string OeisSequence::getBFilePath() const
{
  return getHome() + "b/" + dir_str() + "/" + id_str( "b" ) + ".txt";
}

bool loadBFile( size_t id, const Sequence& seq_full, Sequence& seq_big )
{
  std::string big_path = OeisSequence( id ).getBFilePath();
  std::ifstream big_file( big_path );
  if ( !big_file.good() )
  {
    if ( Log::get().level == Log::Level::DEBUG )
    {
      Log::get().debug( "b-file not found: " + big_path );
    }
    return false;
  }
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
      Log::get().warn( "Unexpected index " + std::to_string( index ) + " in b-file " + big_path );
      return false;
    }
    if ( isCloseToOverflow( value ) )
    {
      break;
    }
    seq_big.push_back( value );
    ++expected_index;
  }

  // align sequences on common prefix (will verify correctness below again!)
  seq_big.align( seq_full, 5 );

  // check length
  if ( seq_big.empty() || seq_big.size() < seq_full.size() )
  {
    Log::get().debug(
        "Sequence in b-file too short: " + big_path + " (" + std::to_string( seq_big.size() ) + "<"
            + std::to_string( seq_full.size() ) + ")" );
    return false;
  }

  // check that the sequences agree on prefix
  Sequence seq_test( std::vector<number_t>( seq_big.begin(), seq_big.begin() + seq_full.size() ) );
  if ( seq_test != seq_full )
  {
    Log::get().warn( "Unexpected terms in b-file " + big_path );
    Log::get().warn( "- expected: " + seq_full.to_string() );
    Log::get().warn( "- found:    " + seq_test.to_string() );
    if ( rand() % 5 == 0 )
    {
      std::remove( big_path.c_str() );
    }
    return false;
  }

  // shrink big sequence to maximum number of terms
  if ( seq_big.size() > OeisSequence::MAX_NUM_TERMS )
  {
    seq_big = Sequence( std::vector<number_t>( seq_big.begin(), seq_big.begin() + OeisSequence::MAX_NUM_TERMS ) );
  }

  if ( Log::get().level == Log::Level::DEBUG )
  {
    Log::get().debug(
        "Loaded b-file for sequence " + std::to_string( id ) + " with " + std::to_string( seq_big.size() ) + " terms" );
  }
  return true;
}

const Sequence& OeisSequence::getFull() const
{
  if ( !attempted_bfile )
  {
    attempted_bfile = true;
    if ( id != 0 )
    {
      Sequence big;
      if ( loadBFile( id, full, big ) )
      {
        // use data from big sequence from now on
        loaded_bfile = true;
        full = big;
      }
    }
  }
  return full;
}

void OeisSequence::fetchBFile() const
{
  if ( !loaded_bfile )
  {
    std::ifstream big_file( getBFilePath() );
    if ( !big_file.good() )
    {
      ensureDir( getBFilePath() );
      std::string cmd = "wget -nv -O " + getBFilePath() + " https://oeis.org/" + id_str() + "/" + id_str( "b" )
          + ".txt";
      if ( system( cmd.c_str() ) != 0 )
      {
        Log::get().error( "Error fetching b-file for " + id_str(), true ); // need to exit here to be able to cancel
      }
      attempted_bfile = false; // reload on next access
    }
  }
}
