#include "oeis_manager.hpp"

#include "config.hpp"
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
#include <time.h>

void throwParseError( const std::string &line )
{
  Log::get().error( "error parsing OEIS line: " + line, true );
}

std::string getStatsHome()
{
  // no trailing / here
  return getLodaHome() + "stats";
}

OeisManager::OeisManager( const Settings &settings, int64_t max_age_in_days, bool force_overwrite ) // magic number
    : settings( settings ),
      overwrite_mode( force_overwrite ? OverwriteMode::ALL : ConfigLoader::load( settings ).overwrite_mode ),
      max_age_in_days( max_age_in_days ),
      interpreter( settings ),
      finder( settings ),
      finder_initialized( false ),
      minimizer( settings ),
      optimizer( settings ),
      loaded_count( 0 ),
      total_count( 0 ),
      stats_loaded( false )
{
}

void OeisManager::load()
{
  // check if already loaded
  if ( total_count > 0 )
  {
    return;
  }

  // first load the deny and protect lists (needs no lock)
  loadList( "deny", deny_list );
  loadList( "overwrite", overwrite_list );
  loadList( "protect", protect_list );

  {
    // obtain lock
    FolderLock lock( OeisSequence::getHome() );

    // update index if needed
    update();

    // load sequence data, names and deny list
    Log::get().info( "Loading sequences from the OEIS index" );
    loadData();
    loadNames();

    // lock released at the end of this block
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
  Log::get().info( "Loaded " + std::to_string( loaded_count ) + "/" + std::to_string( total_count ) + " sequences" );
}

void OeisManager::loadData()
{
  std::string path = OeisSequence::getHome() + "stripped";
  std::ifstream stripped( path );
  if ( !stripped.good() )
  {
    Log::get().error( "OEIS data not found: " + path, true );
  }
  std::string line;
  size_t pos;
  size_t id;
  int64_t num, sign;
  Sequence seq_full, seq_big;

  loaded_count = 0;
  total_count = 0;

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
    total_count++;
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

    // add sequence to index
    if ( id >= sequences.size() )
    {
      sequences.resize( 2 * id );
    }
    sequences[id] = OeisSequence( id, "", seq_full );
    loaded_count++;
  }
}

void OeisManager::loadNames()
{
  Log::get().debug( "Loading sequence names from the OEIS index" );
  std::string path = OeisSequence::getHome() + "names";
  std::ifstream names( path );
  if ( !names.good() )
  {
    Log::get().error( "OEIS data not found: " + path, true );
  }
  std::string line;
  size_t pos;
  size_t id;
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

void OeisManager::loadList( const std::string& name, std::unordered_set<size_t>& list )
{
  Log::get().debug( "Loading " + name + " list" );
  std::string path = "programs/oeis/" + name + ".txt";
  std::ifstream names( path );
  if ( !names.good() )
  {
    Log::get().error( name + " list data not found: " + path, true );
  }
  std::string line, id;
  list.clear();
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
    id = "";
    for ( char ch : line )
    {
      if ( ch == ':' || ch == ';' || ch == ' ' || ch == '\t' || ch == '\n' )
      {
        break;
      }
      id += ch;
    }
    list.insert( OeisSequence( id ).id );
  }
  Log::get().debug( "Finished loading of " + name + " list with " + std::to_string( list.size() ) + " entries" );
}

Finder& OeisManager::getFinder()
{
  if ( !finder_initialized )
  {
    // generate stats is needed
    getStats();

    ignore_list.clear();
    for ( auto& seq : sequences )
    {
      if ( seq.id == 0 )
      {
        continue;
      }
      if ( shouldMatch( seq ) )
      {
        auto seq_norm = seq.getTerms( settings.num_terms );
        finder.insert( seq_norm, seq.id );
      }
      else
      {
        ignore_list.insert( seq.id );
      }
    }
    finder_initialized = true;

    // print summary
    Log::get().info(
        "Initialized " + std::to_string( finder.getMatchers().size() ) + " matchers (ignoring "
            + std::to_string( ignore_list.size() ) + " sequences)" );
    finder.logSummary( loaded_count );
  }
  return finder;
}

bool OeisManager::shouldMatch( const OeisSequence& seq ) const
{
  if ( seq.id == 0 )
  {
    return false;
  }

  // sequence on the deny or protect list?
  if ( deny_list.find( seq.id ) != deny_list.end() )
  {
    return false;
  }
  if ( protect_list.find( seq.id ) != protect_list.end() )
  {
    return false;
  }

  // linear sequence?
  auto terms = seq.getTerms( settings.num_terms );
  if ( !settings.search_linear && terms.is_linear( settings.linear_prefix ) )
  {
    return false;
  }

  // check if program exists
  const bool prog_exists = (seq.id < stats.found_programs.size()) && stats.found_programs[seq.id];

  // decide based on overwrite mode
  switch ( overwrite_mode )
  {
  case OverwriteMode::NONE:
    return !prog_exists;

  case OverwriteMode::ALL:
    return true;

  case OverwriteMode::AUTO:
  {
    if ( !prog_exists )
    {
      return true;
    }
    const bool should_overwrite = overwrite_list.find( seq.id ) != overwrite_list.end();
    const bool is_complex = stats.getTransitiveLength( seq.id, false ) > 10; // magic number
    return is_complex || should_overwrite;
  }
  }
  return true; // unreachable
}

void OeisManager::update()
{
  std::vector<std::string> files = { "stripped", "names" };

  // check which files need to be updated
  auto it = files.begin();
  int64_t age_in_days = -1;
  while ( it != files.end() )
  {
    auto path = OeisSequence::getHome() + *it;
    age_in_days = getFileAgeInDays( path );
    if ( age_in_days >= 0 && age_in_days < max_age_in_days )
    {
      // no need to update this file
      it = files.erase( it );
      continue;
    }
    it++;
  }
  if ( !files.empty() )
  {
    if ( age_in_days == -1 )
    {
      Log::get().info( "Creating OEIS index at " + OeisSequence::getHome() );
      ensureDir( OeisSequence::getHome() );
    }
    else
    {
      Log::get().info( "Updating OEIS index (last update " + std::to_string( age_in_days ) + " days ago)" );
    }
    std::string cmd, path;
    for ( auto &file : files )
    {
      path = OeisSequence::getHome() + file;
      cmd = "wget -nv --no-use-server-timestamps -O " + path + ".gz https://oeis.org/" + file + ".gz";
      if ( system( cmd.c_str() ) != 0 )
      {
        Log::get().error( "Error fetching " + file + " file", true );
      }
      std::ifstream f( OeisSequence::getHome() + file );
      if ( f.good() )
      {
        f.close();
        std::remove( path.c_str() );
      }
      cmd = "gzip -d " + path + ".gz";
      if ( system( cmd.c_str() ) != 0 )
      {
        Log::get().error( "Error unzipping " + path + ".gz", true );
      }
    }
  }
}

void OeisManager::generateStats( int64_t age_in_days )
{
  load();
  std::string msg;
  if ( age_in_days < 0 )
  {
    msg = "Generating program stats";
  }
  else
  {
    msg = "Regenerating program stats (last update " + std::to_string( age_in_days ) + " days ago)";
  }
  Log::get().info( msg );
  stats = Stats();

  size_t num_processed = 0;
  Parser parser;
  Program program;
  std::string file_name;
  bool has_b_file, has_program;

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
      try
      {
        program = parser.parse( program_file );
        has_program = true;
      }
      catch ( const std::exception &exc )
      {
        Log::get().error( "Error parsing " + file_name + ": " + std::string( exc.what() ), false );
        continue;
      }

      ProgramUtil::removeOps( program, Operation::Type::NOP );

      // update stats
      stats.updateProgramStats( s.id, program );
      num_processed++;

      //if ( num_processed % 1000 == 0 )
      //{
      //  Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
      //}
    }
    stats.updateSequenceStats( s.id, has_program, has_b_file );
  }

  // write stats
  stats.save( getStatsHome() );

  Log::get().info( "Finished stats generation for " + std::to_string( num_processed ) + " programs" );
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

void OeisManager::migrate()
{
  for ( size_t id = 1; id <= 400000; id++ )
  {
    OeisSequence s( id );
    auto old_program_path = "programs/oeis/" + s.id_str() + ".asm";
    migrateFile( old_program_path, s.getProgramPath() );
    auto old_b_file_path = OeisSequence::getHome() + "b/" + s.id_str( "b" ) + ".txt";
    migrateFile( old_b_file_path, s.getBFilePath() );
  }
}

const std::vector<OeisSequence>& OeisManager::getSequences() const
{
  return sequences;
}

const Stats& OeisManager::getStats()
{
  if ( !stats_loaded )
  {
    auto home = getStatsHome();

    // obtain lock
    FolderLock lock( home );

    // check age of stats
    auto age_in_days = getFileAgeInDays( stats.getMainStatsFile( home ) );
    if ( age_in_days < 0 || age_in_days >= max_age_in_days )
    {
      generateStats( age_in_days );
    }
    try
    {
      stats.load( home );
    }
    catch ( const std::exception& e )
    {
      Log::get().warn( "Exception during stats loading, regenerating..." );
      generateStats( age_in_days );
      stats.load( home ); // reload
    }
    stats_loaded = true;

    // lock released at the end of this block
  }
  return stats;
}

void OeisManager::addCalComments( Program& p ) const
{
  for ( auto &op : p.ops )
  {
    if ( op.type == Operation::Type::CAL && op.source.type == Operand::Type::CONSTANT )
    {
      auto id = op.source.value;
      if ( id >= 0 && id < (int64_t) sequences.size() && sequences[id].id )
      {
        op.comment = sequences[id].name;
      }
    }
  }
}

void OeisManager::dumpProgram( size_t id, Program p, const std::string &file ) const
{
  ProgramUtil::removeOps( p, Operation::Type::NOP );
  addCalComments( p );
  ensureDir( file );
  std::ofstream out( file );
  auto &seq = sequences.at( id );
  out << "; " << seq << std::endl;
  out << "; " << seq.getTerms( OeisSequence::DEFAULT_SEQ_LENGTH ) << std::endl;
  out << std::endl;
  ProgramUtil::print( p, out );
  out.close();
}

void OeisManager::alert( Program p, size_t id, const std::string& prefix, const std::string& color ) const
{
  auto& seq = sequences.at( id );
  std::stringstream buf;
  buf << prefix << " program for " << seq << " Terms: " << seq.getTerms( settings.num_terms );
  auto msg = buf.str();
  Log::AlertDetails details;
  details.title = seq.id_str();
  details.title_link = seq.url_str();
  details.color = color;
  buf << "\\n\\`\\`\\`\\n";
  ProgramUtil::removeOps( p, Operation::Type::NOP );
  addCalComments( p );
  ProgramUtil::print( p, buf, "\\n" );
  buf << "\\`\\`\\`";
  details.text = buf.str();
  Log::get().alert( msg, details );
}

std::pair<bool, Program> OeisManager::checkAndMinimize( const Program &p, const OeisSequence &seq )
{
  Log::get().debug( "Checking and minimizing program for " + seq.id_str() );

  std::pair<status_t, steps_t> check;
  std::pair<bool, Program> result;
  result.second = p;

  // get the extended sequence
  auto extended_seq = seq.getTerms( OeisSequence::EXTENDED_SEQ_LENGTH );

  // check the program w/o minimization
  check = interpreter.check( p, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id );
  result.first = (check.first != status_t::ERROR); // we allow warnings
  if ( !result.first )
  {
    return result; // not correct
  }
  const auto basic_check_result = check.first;

  // minimize for default number of terms
  minimizer.optimizeAndMinimize( result.second, 2, 1, OeisSequence::DEFAULT_SEQ_LENGTH ); // default length
  check = interpreter.check( result.second, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id );
  result.first = (check.first != status_t::ERROR); // we allow warnings
  if ( result.first )
  {
    return result;
  }

  if ( basic_check_result == status_t::OK )
  {
    // if we got here, there was an error in the minimization
    Log::get().error(
        "Program for " + seq.id_str() + " generates wrong result after minimization with "
            + std::to_string( OeisSequence::DEFAULT_SEQ_LENGTH ) + " terms", false );
    std::string f = getLodaHome() + "debug/minimizer/" + seq.id_str() + ".asm";
    ensureDir( f );
    std::ofstream out( f );
    ProgramUtil::print( p, out );
  }

  return result;
}

size_t getBadOpsCount( const Program& p )
{
  // we prefer programs the following programs:
  // - w/o indirect memory access
  // - w/o loops that have non-constant args
  // - w/o gcd with powers of 2
  size_t num_ops = ProgramUtil::numOps( p, Operand::Type::INDIRECT );
  for ( auto &op : p.ops )
  {
    if ( op.type == Operation::Type::LPB && op.source.type != Operand::Type::CONSTANT )
    {
      num_ops++;
    }
    if ( op.type == Operation::Type::GCD && op.source.type == Operand::Type::CONSTANT )
    {
      auto v = op.source.value;
      if ( getPowerOf( v, 2 ) >= 10 || getPowerOf( v, 3 ) >= 6 || getPowerOf( v, 5 ) >= 5 || getPowerOf( v, 7 ) >= 4
          || getPowerOf( v, 10 ) >= 3 )
      {
        num_ops++;
      }
    }
  }
  return num_ops;
}

std::string OeisManager::isOptimizedBetter( Program existing, Program optimized, const OeisSequence &seq )
{
  // check if there are illegal recursions; do we really need this?!
  for ( auto &op : optimized.ops )
  {
    if ( op.type == Operation::Type::CAL )
    {
      if ( op.source.type != Operand::Type::CONSTANT || op.source.value == static_cast<number_t>( seq.id ) )
      {
        return "";
      }
    }
  }

  // remove nops...
  optimizer.removeNops( existing );
  optimizer.removeNops( optimized );

  // we want at least one operation (avoid empty program for A000004
  if ( optimized.ops.empty() )
  {
    return "";
  }

  auto terms = seq.getTerms( OeisSequence::EXTENDED_SEQ_LENGTH );
  if ( terms.empty() )
  {
    Log::get().error( "Error fetching b-file for " + seq.id_str(), true );
  }

  // compare number of successfully computed terms
  auto optimized_check = interpreter.check( optimized, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id );
  auto existing_check = interpreter.check( existing, terms, OeisSequence::DEFAULT_SEQ_LENGTH, seq.id );
  if ( optimized_check.second.runs > existing_check.second.runs )
  {
    return "Better";
  }
  else if ( optimized_check.second.runs < existing_check.second.runs )
  {
    return ""; // optimized is worse
  }

  // compare number of "bad" operations
  auto optimized_bad_count = getBadOpsCount( optimized );
  auto existing_bad_count = getBadOpsCount( existing );
  if ( optimized_bad_count < existing_bad_count )
  {
    return "Simpler";
  }
  else if ( optimized_bad_count > existing_bad_count )
  {
    return ""; // optimized is worse
  }

  // ...and compare number of execution cycles
  auto existing_cycles = existing_check.second.total;
  auto optimized_cycles = optimized_check.second.total;
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

  return "";
}

std::pair<bool, bool> OeisManager::updateProgram( size_t id, const Program &p )
{
  auto &seq = sequences.at( id );
  std::string file_name = seq.getProgramPath();
  bool is_new = true;
  std::string change;

  // minimize and check the program
  auto minimized = checkAndMinimize( p, seq );
  if ( !minimized.first )
  {
    return
    { false,false};
  }

  // check if there is an existing program already
  {
    std::ifstream in( file_name );
    if ( in.good() )
    {
      if ( ignore_list.find( id ) == ignore_list.end() )
      {
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
        change = isOptimizedBetter( existing, minimized.second, id );
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

  // write new or optimized program version
  dumpProgram( id, minimized.second, file_name );

  // send alert
  std::string prefix = is_new ? "First" : change;
  std::string color = is_new ? "good" : "warning";
  alert( minimized.second, id, prefix, color );

  return
  { true,is_new};
}
