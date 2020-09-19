#include "oeis.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "stats.hpp"
#include "util.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdlib.h>

size_t Oeis::MAX_NUM_TERMS = 250;

std::string getOeisHome()
{
  // don't remove the trailing /
  return getLodaHome() + "oeis/";
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
  return getOeisHome() + "b/" + dir_str() + "/" + id_str( "b" ) + ".txt";
}

void throwParseError( const std::string &line )
{
  Log::get().error( "error parsing OEIS line: " + line, true );
}

Oeis::Oeis( const Settings &settings )
    :
    settings( settings ),
    interpreter( settings ),
    finder( settings ),
    minimizer( settings ),
    optimizer( settings ),
    total_count_( 0 )
{
}

void Oeis::load( volatile sig_atomic_t &exit_flag )
{
  // check if already loaded
  if ( total_count_ > 0 )
  {
    return;
  }
  // load sequence data
  Log::get().info( "Loading sequences from the OEIS index" );
  std::ifstream stripped( getOeisHome() + "stripped" );
  if ( !stripped.good() )
  {
    Log::get().error( "OEIS data not found: run \"loda update\" to download it", true );
  }
  std::string line;
  size_t pos;
  size_t id;
  int64_t num, sign;
  Sequence seq_full, seq_norm, seq_big;
  size_t loaded_count = 0;
  size_t big_loaded_count = 0;
  while ( std::getline( stripped, line ) )
  {
    if ( exit_flag )
    {
      break;
    }
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
    sign = 1;
    seq_full.clear();
    while ( pos < line.length() )
    {
      if ( line[pos] == ',' )
      {
        seq_full.push_back( sign * num );
        num = 0;
        sign = 1;
      }
      else if ( line[pos] >= '0' && line[pos] <= '9' )
      {
        if ( isCloseToOverflow( num ) )
        {
          break;
        }
        num = (10 * num) + (line[pos] - '0');
      }
      else if ( line[pos] == '-' )
      {
        sign = -1;
      }
      else
      {
        throwParseError( line );
      }
      ++pos;
    }

    // check minimum number of terms
    if ( seq_full.size() < settings.num_terms )
    {
      continue;
    }

    // normalized sequence
    seq_norm = Sequence( std::vector<number_t>( seq_full.begin(), seq_full.begin() + settings.num_terms ) );

    // big sequence
    seq_big.clear();
    std::string big_path = OeisSequence( id ).getBFilePath();
    std::ifstream big_file( big_path );
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
          Log::get().warn( "Unexpected index " + std::to_string( index ) + " in b-file " + big_path );
          seq_big.clear();
          break;
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
      if ( seq_big.size() < seq_full.size() )
      {
        Log::get().debug(
            "Sequence in b-file too short: " + big_path + " (" + std::to_string( seq_big.size() ) + "<"
                + std::to_string( seq_full.size() ) + ")" );
        seq_big.clear();
      }
      else
      {
        // check that the sequences agree on prefix
        Sequence seq_test( std::vector<number_t>( seq_big.begin(), seq_big.begin() + seq_full.size() ) );
        if ( seq_test != seq_full )
        {
          Log::get().warn( "Unexpected terms in b-file " + big_path );
          Log::get().warn( "- expected: " + seq_full.to_string() );
          Log::get().warn( "- found:    " + seq_test.to_string() );
          seq_big.clear();
        }
      }

      // shrink big sequence to maximum number of terms
      if ( seq_big.size() > MAX_NUM_TERMS )
      {
        seq_big = Sequence( std::vector<number_t>( seq_big.begin(), seq_big.begin() + MAX_NUM_TERMS ) );
      }

      // use data from big sequence from now on
      if ( !seq_big.empty() )
      {
        big_loaded_count++;
        seq_full = seq_big;
        if ( Log::get().level == Log::Level::DEBUG )
        {
          Log::get().debug(
              "Loaded b-file for sequence " + std::to_string( id ) + " with " + std::to_string( seq_big.size() )
                  + " terms" );
        }
      }
    }
    else if ( Log::get().level == Log::Level::DEBUG )
    {
      Log::get().debug( "b-file not found: " + big_path );
    }

    // add sequence to index
    if ( id >= sequences.size() )
    {
      sequences.resize( 2 * id );
    }
    sequences[id] = OeisSequence( id, "", seq_norm, seq_full );

    // add sequences to matchers
    finder.insert( seq_norm, id );

    ++loaded_count;
  }

  loadNames( exit_flag );

  // remove known sequences if they should be ignored
  if ( !settings.optimize_existing_programs )
  {
    std::vector<number_t> seqs_to_remove;
    for ( auto &seq : sequences )
    {
      if ( seq.id == 0 )
      {
        continue;
      }
      std::ifstream in( seq.getProgramPath() );
      if ( in.good() )
      {
        seqs_to_remove.push_back( seq.id );
        in.close();
      }
    }
    if ( !seqs_to_remove.empty() )
    {
      Log::get().info(
          "Ignoring " + std::to_string( seqs_to_remove.size() )
              + " sequences because programs exist for them already" );
      for ( auto id : seqs_to_remove )
      {
        removeSequence( id );
      }
    }
  }

  // shrink sequences vector again
  if ( !sequences.empty() )
  {
    size_t i;
    for ( i = sequences.size() - 1; i > 0; i-- )
    {
      if ( sequences[i].id != 0 )
      {
        break;
      }
    }
    sequences.resize( i + 1 );
  }

  // print summary
  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count_ ) + " sequences and "
          + std::to_string( big_loaded_count ) + " b-files" );
  std::stringstream buf;
  buf << "Matcher compaction ratios: ";
  for ( size_t i = 0; i < finder.getMatchers().size(); i++ )
  {
    if ( i > 0 ) buf << ", ";
    double ratio = 100.0 * (double) finder.getMatchers()[i]->getReducedSequences().size() / (double) loaded_count;
    buf << finder.getMatchers()[i]->getName() << ": " << std::setprecision( 4 ) << ratio << "%";
  }
  Log::get().info( buf.str() );

}

void Oeis::loadNames( volatile sig_atomic_t &exit_flag )
{
  Log::get().debug( "Loading sequence names from the OEIS index" );
  std::ifstream names( getOeisHome() + "names" );
  if ( !names.good() )
  {
    Log::get().error( "OEIS data not found: run \"loda update\" to download it", true );
  }
  std::string line;
  size_t pos;
  size_t id;
  while ( std::getline( names, line ) )
  {
    if ( exit_flag )
    {
      break;
    }
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

void Oeis::update( volatile sig_atomic_t &exit_flag )
{
  if ( !settings.optimize_existing_programs )
  {
    Log::get().error( "Option -x required to run update", true );
  }
  Log::get().info( "Updating OEIS index" );
  ensureDir( getOeisHome() );
  std::string cmd, path;
  int exit_code;
  std::vector<std::string> files = { "stripped", "names" };
  for ( auto &file : files )
  {
    if ( exit_flag )
    {
      break;
    }
    path = getOeisHome() + file;
    cmd = "wget -nv -O " + path + ".gz https://oeis.org/" + file + ".gz";
    exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error fetching " + file + " file", true );
    }
    std::ifstream f( getOeisHome() + file );
    if ( f.good() )
    {
      f.close();
      std::remove( path.c_str() );
    }
    cmd = "gzip -d " + path + ".gz";
    exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error unzipping " + path + ".gz", true );
    }
  }
  load( exit_flag );
  Stats stats;
  stats.load( "stats" );
  for ( auto &s : sequences )
  {
    if ( exit_flag )
    {
      break;
    }
    if ( s.id == 0 )
    {
      continue;
    }
    std::ifstream program_file( s.getProgramPath() );
    std::ifstream b_file( s.getBFilePath() );
    if ( !b_file.good()
        && (program_file.good() || (stats.cached_b_files.size() > (size_t) s.id && stats.cached_b_files[s.id])) )
    {
      ensureDir( s.getBFilePath() );
      cmd = "wget -nv -O " + s.getBFilePath() + " https://oeis.org/" + s.id_str() + "/" + s.id_str( "b" ) + ".txt";
      exit_code = system( cmd.c_str() );
      if ( exit_code != 0 )
      {
        Log::get().error( "Error fetching b-file for " + s.id_str(), true );
      }
    }
    b_file.close();
    program_file.close();
  }
  Log::get().info( "Finished update" );
}

void migrateFile( const std::string &from, const std::string &to )
{
  std::ifstream f( from );
  if ( f.good() )
  {
    Log::get().warn( "Migrating " + from + " -> " + to );
    f.close();
    ensureDir( to );
    auto cmd = "mv " + from + " " + to;
    auto exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error moving file " + from, true );
    }
  }
}

void Oeis::migrate( volatile sig_atomic_t &exit_flag )
{
  for ( size_t id = 1; id <= 400000; id++ )
  {
    if ( exit_flag )
    {
      break;
    }
    OeisSequence s( id );
    auto old_program_path = "programs/oeis/" + s.id_str() + ".asm";
    migrateFile( old_program_path, s.getProgramPath() );
    auto old_b_file_path = getOeisHome() + "b/" + s.id_str( "b" ) + ".txt";
    migrateFile( old_b_file_path, s.getBFilePath() );
  }
}

const std::vector<OeisSequence>& Oeis::getSequences() const
{
  return sequences;
}

void Oeis::removeSequence( size_t id )
{
  if ( id >= sequences.size() )
  {
    return;
  }
  if ( sequences[id].id == id )
  {
    finder.remove( sequences[id], id );
    sequences[id] = OeisSequence();
  }
}

void Oeis::dumpProgram( size_t id, Program p, const std::string &file ) const
{
  ProgramUtil::removeOps( p, Operation::Type::NOP );
  ensureDir( file );
  std::ofstream out( file );
  auto &seq = sequences.at( id );
  out << "; " << seq << std::endl;
  out << "; " << seq.full << std::endl;
  out << std::endl;
  ProgramUtil::print( p, out );
}

std::pair<bool, Program> Oeis::minimizeAndCheck( const Program &p, const OeisSequence &seq, bool minimize ) const
{
  // optimize and minimize program
  std::pair<bool, Program> minimized;
  minimized.first = true;
  minimized.second = p;
  if ( minimize )
  {
    minimizer.optimizeAndMinimize( minimized.second, 2, 1, seq.full.size() );
  }

  // check its correctness
  Sequence new_seq;
  try
  {
    interpreter.eval( minimized.second, new_seq, seq.full.size() );
    if ( seq.full.size() != new_seq.size() || seq.full != new_seq )
    {
      minimized.first = false;
    }
  }
  catch ( const std::exception &e )
  {
    minimized.first = false;
  }

  // throw error if not correct
  if ( !minimized.first )
  {
    std::string msg = "Program for " + seq.id_str() + " generates wrong result";
    if ( minimize )
    {
      msg = msg + " after optimization or minimization";
    }
    Log::get().error( msg, false );
    std::string f = "programs/debug/optimizer/" + seq.id_str() + ".asm";
    ensureDir( f );
    std::ofstream out( f );
    ProgramUtil::print( p, out );
  }

  return minimized;
}

int Oeis::getNumCycles( const Program &p ) const
{
  Memory mem;
  const number_t input = settings.num_terms - 1;
  mem.set( 0, input );
  try
  {
    return interpreter.run( p, mem );
  }
  catch ( const std::exception &e )
  {
    auto timestamp =
        std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count()
                % 1000000 );
    std::string f = "programs/debug/interpreter/" + timestamp + ".asm";
    ensureDir( f );
    std::ofstream o( f );
    ProgramUtil::print( p, o );
    o.close();
    Log::get().error( "Error evaluating program for n=" + std::to_string( input ) + ": " + std::string( e.what() ),
        true );
  }
  return -1;
}

std::string Oeis::isOptimizedBetter( Program existing, Program optimized ) const
{
  // we prefer programs w/o indirect memory access
  auto in_opt = ProgramUtil::numOps( optimized, Operand::Type::INDIRECT );
  auto in_ext = ProgramUtil::numOps( existing, Operand::Type::INDIRECT );
  if ( in_opt < in_ext )
  {
    return "Simpler";
  }
  else if ( in_opt > in_ext )
  {
    return "";
  }

  // now remove nops...
  optimizer.removeNops( existing );
  optimizer.removeNops( optimized );

  // ...and compare number of execution cycles
  auto existing_cycles = getNumCycles( existing );
  auto optimized_cycles = getNumCycles( optimized );
  if ( existing_cycles >= 0 && optimized_cycles >= 0 )
  {
    if ( optimized_cycles < existing_cycles )
    {
      return "Faster";
    }
    else if ( optimized_cycles > existing_cycles )
    {
      return "";
    }
  }

  // shorter programs are better (nops have been removed already at this point)
  if ( ProgramUtil::numOps( optimized, true ) < ProgramUtil::numOps( existing, true ) )
  {
    return "Shorter";
  }
  return "";
}

std::pair<bool, bool> Oeis::updateProgram( size_t id, const Program &p ) const
{
  auto &seq = sequences.at( id );
  std::string file_name = seq.getProgramPath();
  bool is_new = true;
  std::string change;
  std::pair<bool, Program> optimized;
  {
    std::ifstream in( file_name );
    if ( in.good() )
    {
      if ( settings.optimize_existing_programs )
      {
        optimized = minimizeAndCheck( p, seq, true );
        if ( !optimized.first )
        {
          return
          { false,false};
        }
        is_new = false;
        Parser parser;
        Program existing;
        try
        {
          existing = parser.parse( in );
        }
        catch ( const std::exception &exc )
        {
          Log::get().error( "Error parsing " + file_name, false );
          return
          { false,false};
        }
        change = isOptimizedBetter( existing, optimized.second );
        if ( change.empty() )
        {
          return
          { false,false};
        }
      }
      else
      {
        return
        { false,false};
      }
    }
  }
  if ( is_new )
  {
    optimized = minimizeAndCheck( p, seq, false );
    if ( !optimized.first )
    {
      return
      { false,false};
    }
  }
  std::stringstream buf;
  if ( is_new ) buf << "First";
  else buf << change;
  buf << " program for " << seq << " Terms: " << static_cast<Sequence>( seq );
  Log::get().alert( buf.str() );
  dumpProgram( id, optimized.second, file_name );
  return
  { true,is_new};
}

void Oeis::maintain( volatile sig_atomic_t &exit_flag )
{
  if ( !settings.optimize_existing_programs )
  {
    Log::get().error( "Option -x required to run maintenance", true );
  }
  load( exit_flag );
  Log::get().info( "Start maintaining OEIS programs" );
  std::vector<std::stringstream> list_files( 10 );
  Stats stats;
  size_t num_optimized = 0;
  Parser parser;
  Program program, optimized;
  Sequence result;
  std::string file_name;
  bool has_b_file, has_program, is_okay;

  for ( auto &s : sequences )
  {
    if ( s.id == 0 )
    {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file( file_name );
    std::ifstream b_file( s.getBFilePath() );
    has_b_file = b_file.good();
    has_program = false;
    if ( program_file.good() )
    {
      if ( exit_flag ) continue;
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Checking program for " + s.to_string() );
      }
      try
      {
        program = parser.parse( program_file );
      }
      catch ( const std::exception &exc )
      {
        Log::get().error( "Error parsing " + file_name, false );
        continue;
      }
      try
      {
        interpreter.eval( program, result, s.full.size() );
        is_okay = (result == s.full);
      }
      catch ( const std::exception &exc )
      {
        is_okay = false;
      }
      if ( !is_okay )
      {
        Log::get().alert( "Removing invalid program for " + s.to_string() );
        program_file.close();
        remove( file_name.c_str() );
      }
      else
      {
        has_program = true;
        ProgramUtil::removeOps( program, Operation::Type::NOP );
        optimized = program;
        minimizer.optimizeAndMinimize( optimized, 2, 1, s.full.size() );
        if ( program != optimized )
        {
          Log::get().warn( "Updating program because it is not optimal: " + file_name );
          num_optimized++;
        }
        dumpProgram( s.id, optimized, file_name );

        // collect stats
        stats.updateProgram( optimized );

        // write list file
        size_t list_index = s.id / 100000;
        size_t num_ops = ProgramUtil::numOps( program, false );
        list_files.at( list_index ) << "* [" << s.id_str() << "](http://oeis.org/" << s.id_str() << ") ([L"
            << std::setw( 2 ) << std::setfill( '0' ) << num_ops << " program](" << s.dir_str() << "/" << s.id_str()
            << ".asm)): " << s.name << "\n";
      }
    }
    stats.updateSequence( s.id, has_program, has_b_file );
  }

  // write stats
  stats.save( "stats" );

  // write lists
  for ( size_t i = 0; i < list_files.size(); i++ )
  {
    auto buf = list_files[i].str();
    if ( !buf.empty() )
    {
      std::string list_path = "programs/oeis/list" + std::to_string( i ) + ".md";
      OeisSequence start( (i * 100000) + 1 );
      OeisSequence end( (i + 1) * 100000 );
      std::ofstream list_file( list_path );
      list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
      list_file
          << "List of integer sequences with links to LODA programs. An _Ln_ program is a LODA program of length _n_."
          << "\n\n";
      list_file << buf;
    }
  }

  if ( num_optimized > 0 )
  {
    Log::get().alert(
        "Optimized " + std::to_string( num_optimized ) + "/" + std::to_string( stats.num_programs ) + " programs." );
  }
  Log::get().info( "Finished maintaining programs" );
}
