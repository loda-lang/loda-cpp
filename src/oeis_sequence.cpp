#include "oeis_sequence.hpp"

#include "parser.hpp"
#include "util.hpp"

#include <iomanip>
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
  const OeisSequence oeis_seq( id );

  // try to read b-file
  std::ifstream big_file( oeis_seq.getBFilePath() );
  if ( big_file.good() )
  {
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
      if ( isCloseToOverflow( value ) )
      {
        break;
      }
      seq_big.push_back( value );
      ++expected_index;
    }
    Log::get().debug( "Read b-file for " + oeis_seq.id_str() + " with " + std::to_string( seq_big.size() ) + " terms" );
  }
  else
  {
    // if b-file not found, try to parse comment from existing program
    std::ifstream program_file( oeis_seq.getProgramPath() );
    if ( program_file.good() )
    {
      try
      {
        Parser parser;
        auto p = parser.parse( oeis_seq.getProgramPath() );
        if ( p.ops.size() > 1 && p.ops[1].type == Operation::Type::NOP && !p.ops[1].comment.empty() )
        {
          std::stringstream ss( p.ops[1].comment );
          parser.in = &ss;
          while ( true )
          {
            ss >> std::ws;
            auto c = ss.peek();
            if ( c == EOF )
            {
              break;
            }
            seq_big.push_back( parser.readValue() );
            c = ss.peek();
            if ( c == EOF )
            {
              break;
            }
            parser.readSeparator( ',' );
          }
        }
        // if the number of terms in the comment is less than MAX_NUM_TERMS,
        // we can't be sure it's coming from the b-file. thus we discard it in this case.
        if ( seq_big.size() != OeisSequence::MAX_NUM_TERMS )
        {
          seq_big.clear();
        }
        else
        {
          Log::get().debug(
              "Read sequence from program for " + oeis_seq.id_str() + " with " + std::to_string( seq_big.size() )
                  + " terms" );
        }
      }
      catch ( const std::exception& e )
      {
        Log::get().warn( "Failed to extract terms from " + oeis_seq.getProgramPath() + "; falling back to b-file" );
      }
    }
  }

  // not found?
  if ( seq_big.empty() )
  {
    if ( Log::get().level == Log::Level::DEBUG )
    {
      Log::get().debug( "Neither b-file nor program found: " + oeis_seq.id_str() );
    }
    return false;
  }

  // align sequences on common prefix (will verify correctness below again!)
  seq_big.align( seq_full, 5 );

  // check length
  if ( seq_big.empty() || seq_big.size() < seq_full.size() )
  {
    Log::get().debug(
        "Sequence in b-file too short for " + oeis_seq.id_str() + " (" + std::to_string( seq_big.size() ) + "<"
            + std::to_string( seq_full.size() ) + ")" );
    return false;
  }

  // check that the sequences agree on prefix
  Sequence seq_test( std::vector<number_t>( seq_big.begin(), seq_big.begin() + seq_full.size() ) );
  if ( seq_test != seq_full )
  {
    Log::get().warn( "Unexpected terms in b-file or program for " + oeis_seq.id_str() );
    Log::get().warn( "- expected: " + seq_full.to_string() );
    Log::get().warn( "- found:    " + seq_test.to_string() );
    if ( rand() % 5 == 0 )
    {
      Log::get().debug( "Removing " + oeis_seq.getBFilePath() );
      std::remove( oeis_seq.getBFilePath().c_str() );
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
        "Loaded long version of sequence " + oeis_seq.id_str() + " with " + std::to_string( seq_big.size() )
            + " terms" );
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
  if ( !attempted_bfile )
  {
    getFull();
  }
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
