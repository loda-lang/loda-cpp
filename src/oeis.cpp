#include "oeis.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
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

std::string getHome()
{
  // don't remove the trailing /
  return std::string( std::getenv( "HOME" ) ) + "/.loda/oeis/";
}

void ensureDir( const std::string &path )
{
  auto index = path.find_last_of( "/" );
  if ( index != std::string::npos )
  {
    auto dir = path.substr( 0, index );
    auto cmd = "mkdir -p " + dir;
    auto exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error creating directory " + dir, true );
    }
  }
  else
  {
    Log::get().error( "Error determining directory for " + path, true );
  }
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

void throwParseError( const std::string &line )
{
  Log::get().error( "error parsing OEIS line: " + line, true );
}

Oeis::Oeis( const Settings &settings )
    :
    settings( settings ),
    interpreter( settings ),
    optimizer( settings ),
    total_count_( 0 )
{
  if ( settings.optimize_existing_programs )
  {
    matchers.resize( 3 );
    matchers[0].reset( new DirectMatcher( false ) );
    matchers[1].reset( new LinearMatcher( false ) );
    matchers[2].reset( new LinearMatcher2( false ) );
  }
  else
  {
    matchers.resize( 5 );
    matchers[0].reset( new DirectMatcher( true ) );
    matchers[1].reset( new LinearMatcher( true ) );
    matchers[2].reset( new LinearMatcher2( true ) );
    matchers[3].reset( new PolynomialMatcher( true ) );
    matchers[4].reset( new DeltaMatcher( true ) );
  }
  matcher_stats.resize( matchers.size() );
  for ( auto &s : matcher_stats )
  {
    s.candidates = 0;
    s.false_positives = 0;
    s.errors = 0;
  }
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
  std::ifstream stripped( getHome() + "stripped" );
  if ( !stripped.good() )
  {
    Log::get().error( "OEIS data not found: run \"loda update\" to download it", true );
  }
  std::string line;
  size_t pos;
  size_t id;
  int64_t num;
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
    seq_full.clear();
    while ( pos < line.length() )
    {
      if ( line[pos] == ',' )
      {
        seq_full.push_back( num );
        num = 0;
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
        seq_full.clear();
        break;
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
        if ( value < 0 )
        {
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
    for ( auto &matcher : matchers )
    {
      matcher->insert( seq_norm, id );
    }

    ++loaded_count;
  }

  loadNames( exit_flag );

  if ( !settings.optimize_existing_programs )
  {
    std::vector<number_t> seqs_to_remove;
    for ( auto &seq : sequences )
    {
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

  Log::get().info(
      "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count_ ) + " sequences and "
          + std::to_string( big_loaded_count ) + " b-files" );
  std::stringstream buf;
  buf << "Matcher compaction ratios: ";
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    if ( i > 0 ) buf << ", ";
    double ratio = 100.0 * (double) matchers[i]->getReducedSequences().size() / (double) loaded_count;
    buf << matchers[i]->getName() << ": " << std::setprecision( 4 ) << ratio << "%";
  }
  Log::get().info( buf.str() );

}

void Oeis::loadNames( volatile sig_atomic_t &exit_flag )
{
  Log::get().debug( "Loading sequence names from the OEIS index" );
  std::ifstream names( getHome() + "names" );
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
  ensureDir( getHome() );
  std::string cmd, path;
  int exit_code;
  std::vector<std::string> files = { "stripped", "names" };
  for ( auto &file : files )
  {
    if ( exit_flag )
    {
      break;
    }
    path = getHome() + file;
    cmd = "wget -nv -O " + path + ".gz https://oeis.org/" + file + ".gz";
    exit_code = system( cmd.c_str() );
    if ( exit_code != 0 )
    {
      Log::get().error( "Error fetching " + file + " file", true );
    }
    std::ifstream f( getHome() + file );
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
  ProgramUtil::Stats stats;
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
    auto old_b_file_path = getHome() + "b/" + s.id_str( "b" ) + ".txt";
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
    for ( auto &matcher : matchers )
    {
      matcher->remove( sequences[id], id );
    }
    sequences[id] = OeisSequence();
  }
}

Matcher::seq_programs_t Oeis::findSequence( const Program &p, Sequence &norm_seq ) const
{
  std::vector<Sequence> seqs;
  seqs.resize( std::max<size_t>( 2, settings.max_index + 1 ) );
  Matcher::seq_programs_t result;
  try
  {
    interpreter.eval( p, seqs );
    norm_seq = seqs[1];
  }
  catch ( const std::exception& )
  {
    return result;
  }
  Program p2 = p;
  p2.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 1, Operand::Type::DIRECT, 0 );
  for ( size_t i = 0; i < seqs.size(); i++ )
  {
    if ( settings.search_linear || !seqs[i].is_linear() )
    {
      if ( i == 1 )
      {
        findAll( p, seqs[i], result );
      }
      else
      {
        p2.ops.back().source.value = i;
        findAll( p2, seqs[i], result );
      }
    }
  }
  return result;
}

void Oeis::findAll( const Program &p, const Sequence &norm_seq, Matcher::seq_programs_t &result ) const
{
  // collect possible matches
  Matcher::seq_programs_t temp_result;
  Sequence full_seq;
  for ( size_t i = 0; i < matchers.size(); i++ )
  {
    temp_result.clear();
    matchers[i]->match( p, norm_seq, temp_result );

    // validate the found matches
    size_t j = 0;
    for ( auto t : temp_result )
    {
      matcher_stats[i].candidates++;
      auto &expected_full_seq = sequences.at( t.first ).full;
      try
      {
        full_seq.clear();
        interpreter.eval( t.second, full_seq, expected_full_seq.size() );
        if ( full_seq.size() != expected_full_seq.size() || full_seq != expected_full_seq )
        {
          matcher_stats[i].false_positives++;
          auto match_length = norm_seq.size();
          auto got = full_seq.subsequence( 0, match_length );
          auto exp = expected_full_seq.subsequence( 0, match_length );
          if ( got != exp )
          {
            auto id = sequences.at( t.first ).id_str();
            Log::get().error( matchers[i]->getName() + " matcher generates wrong program for " + id, false );
            Log::get().error( " -  expected: " + exp.to_string() );
            Log::get().error( " -       got: " + got.to_string() );
            Log::get().error( " - generated: " + norm_seq.to_string() );
            std::string f = "programs/debug/matcher/" + id + ".asm";
            ensureDir( f );
            std::ofstream o1( f );
            ProgramUtil::print( p, o1 );
            std::ofstream o2(
                "programs/debug/matcher/" + id + "-" + matchers[i]->getName() + "-" + std::to_string( j ) + ".asm" );
            ProgramUtil::print( t.second, o2 );
          }
        }
        else
        {
          // successful match!
          result.push_back( t );
        }
      }
      catch ( const std::exception& )
      {
        matcher_stats[i].errors++;
      }
      j++;
    }
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

std::pair<bool, Program> Oeis::optimizeAndCheck( const Program &p, const OeisSequence &seq, bool optimize ) const
{
  // optimize and minimize program
  std::pair<bool, Program> optimized;
  optimized.first = true;
  optimized.second = p;
  if ( optimize )
  {
    optimizer.optimizeAndMinimize( optimized.second, 2, 1, seq.full.size() );
  }

  // check its correctness
  Sequence new_seq;
  try
  {
    interpreter.eval( optimized.second, new_seq, seq.full.size() );
    if ( seq.full.size() != new_seq.size() || seq.full != new_seq )
    {
      optimized.first = false;
    }
  }
  catch ( const std::exception &e )
  {
    optimized.first = false;
  }

  // throw error if not correct
  if ( !optimized.first )
  {
    std::string msg = "Program for " + seq.id_str() + " generates wrong result";
    if ( optimize )
    {
      msg = msg + " after optimization or minimization";
    }
    Log::get().error( msg, false );
    std::string f = "programs/debug/optimizer/" + seq.id_str() + ".asm";
    ensureDir( f );
    std::ofstream out( f );
    ProgramUtil::print( p, out );
  }

  return optimized;
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
    Log::get().error( "Error evaluating program for n=" + std::to_string( input ) + ": " + f, false );
    // TODO: input might be to large
  }
  return -1;
}

std::string Oeis::isOptimizedBetter( Program existing, Program optimized ) const
{
  // we prefer programs w/o indirect memory access
  if ( ProgramUtil::numOps( optimized, Operand::Type::INDIRECT )
      < ProgramUtil::numOps( existing, Operand::Type::INDIRECT ) )
  {
    return "Simpler";
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
        optimized = optimizeAndCheck( p, seq, true );
        if ( !optimized.first )
        {
          return
          { false,false};
        }
        is_new = false;
        Parser parser;
        auto existing = parser.parse( in );
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
    optimized = optimizeAndCheck( p, seq, false );
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
  std::ofstream gen_args;
  gen_args.open( "programs/oeis/generator_args.txt", std::ios_base::app );
  gen_args << seq.id_str() << ": " << settings.getGeneratorArgs() << std::endl;
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
  const std::string readme( "README.md" );
  std::ifstream readme_in( readme );
  std::stringstream buffer;
  std::string str;
  while ( std::getline( readme_in, str ) )
  {
    buffer << str << std::endl;
    if ( str == "## Available Programs" )
    {
      break;
    }
  }
  readme_in.close();
  std::ofstream readme_out( readme );
  readme_out << buffer.str() << std::endl;
  std::ofstream list_file;
  int list_index = -1;
  ProgramUtil::Stats stats;
  size_t num_optimized = 0;
  for ( auto &s : sequences )
  {
    std::string file_name = s.getProgramPath();
    std::ifstream program_file( file_name );
    std::ifstream b_file( s.getBFilePath() );
    bool has_b_file = b_file.good();
    bool has_program = false;
    if ( program_file.good() )
    {
      if ( exit_flag ) continue;
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Checking program for " + s.to_string() );
      }
      Parser parser;
      Program program = parser.parse( program_file );
      Settings settings2 = settings;
      settings2.num_terms = s.full.size();
      Interpreter interpreter( settings2 );
      bool okay;
      try
      {
        Sequence result;
        interpreter.eval( program, result );
        if ( result.size() != s.full.size() || result != s.full )
        {
          Log::get().error( "Program did not evaluate to expected sequence: " + file_name );
          okay = false;
        }
        else
        {
          okay = true;
        }
      }
      catch ( const std::exception &exc )
      {
        okay = false;
      }
      if ( !okay )
      {
        Log::get().warn( "Deleting program due to evaluation error: " + file_name );
        program_file.close();
        remove( file_name.c_str() );
      }
      else
      {
        has_program = true;
        ProgramUtil::removeOps( program, Operation::Type::NOP );
        Program optimized = program;
        Optimizer optimizer( settings2 );
        optimizer.optimizeAndMinimize( optimized, 2, 1, s.full.size() );
        if ( !(program == optimized) )
        {
          Log::get().warn( "Updating program because it is not optimal: " + file_name );
          num_optimized++;
        }
        dumpProgram( s.id, optimized, file_name );

        // collect stats
        stats.updateProgram( optimized );

        // write list file
        if ( list_index < 0 || (int) s.id / 100000 != list_index )
        {
          list_index++;
          std::string list_path = "programs/oeis/list" + std::to_string( list_index ) + ".md";
          OeisSequence start( (list_index * 100000) + 1 );
          OeisSequence end( (list_index + 1) * 100000 );
          readme_out << "* [" << start.id_str() << "-" << end.id_str() << "](" << list_path << ")\n";
          list_file.close();
          list_file.open( list_path );
          list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
          list_file
              << "List of integer sequences with links to LODA programs. An _Ln_ program is a LODA program of length _n_."
              << "\n\n";
        }
        const size_t num_ops = ProgramUtil::numOps( program, false );
        list_file << "* [" << s.id_str() << "](http://oeis.org/" << s.id_str() << ") ([L" << std::setw( 2 )
            << std::setfill( '0' ) << num_ops << " program](" << s.dir_str() << "/" << s.id_str() << ".asm)): "
            << s.name << "\n";
      }
    }
    stats.updateSequence( s.id, has_program, has_b_file );
  }
  list_file.close();
  readme_out << "\n" << "Total number of programs: ";
  readme_out << stats.num_programs << "/" << total_count_ << " (" << (int) (100 * stats.num_programs / total_count_)
      << "%)\n\n";
  readme_out
      << "![LODA Program Length Distribution](https://raw.githubusercontent.com/ckrause/loda/master/stats/lengths.png)\n";
// readme_out << "![LODA Program Counts](https://raw.githubusercontent.com/ckrause/loda/master/stats/counts.png)\n";
  readme_out.close();

// write stats
  stats.save( "stats" );

  if ( num_optimized > 0 )
  {
    Log::get().alert( "Optimized " + std::to_string( num_optimized ) + " programs" );
  }
  Log::get().info( "Finished maintaining programs" );
}
