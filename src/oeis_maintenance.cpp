#include "oeis_maintenance.hpp"

#include "parser.hpp"
#include "program_util.hpp"
#include "stats.hpp"

#include <iomanip>
#include <fstream>
#include <sstream>

OeisMaintenance::OeisMaintenance( const Settings &settings )
    : settings( settings ),
      interpreter( settings ),
      manager( settings )
{
}

void OeisMaintenance::maintain()
{
  if ( !settings.optimize_existing_programs )
  {
    Log::get().error( "Option -x required to run maintenance", true );
  }

  // load sequence data
  manager.load();

  // generate stats
  generateStats( steps_t() );

  // remove invalid programs
  auto num_removed = checkPrograms();

  // minimize programs
  auto num_minimized = minimizePrograms();

  // generate stats again if there was a change
  if ( num_removed > 0 || num_minimized > 0 )
  {
    generateStats( steps_t() );
  }
  Log::get().info( "Finished maintenance of programs" );
}

void OeisMaintenance::generateStats( const steps_t& steps )
{
  manager.load();
  Log::get().info( "Generating program stats" );
  const size_t list_file_size = 50000;
  std::vector<std::stringstream> list_files( 1000000 / list_file_size );
  Stats stats;
  size_t num_processed = 0;
  Parser parser;
  Program program;
  std::string file_name;
  bool has_b_file, has_program;

  for ( auto &s : manager.sequences )
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

      // collect stats
      stats.updateProgramStats( program );

      // write list file
      size_t list_index = s.id / list_file_size;
      size_t num_ops = ProgramUtil::numOps( program, false );
      list_files.at( list_index ) << "* [" << s.id_str() << "](http://oeis.org/" << s.id_str() << ") ([L"
          << std::setw( 2 ) << std::setfill( '0' ) << num_ops << " program](" << s.dir_str() << "/" << s.id_str()
          << ".asm)): " << s.name << "\n";

      if ( ++num_processed % 1000 == 0 )
      {
        Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
      }
    }
    stats.updateSequenceStats( s.id, has_program, has_b_file );
  }

  // write stats
  Log::get().info( "Updating stats and program lists" );
  stats.save( "stats" );

  // write lists
  for ( size_t i = 0; i < list_files.size(); i++ )
  {
    auto buf = list_files[i].str();
    if ( !buf.empty() )
    {
      std::string list_path = "programs/oeis/list" + std::to_string( i ) + ".md";
      OeisSequence start( (i * list_file_size) + 1 );
      OeisSequence end( (i + 1) * list_file_size );
      std::ofstream list_file( list_path );
      list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
      list_file
          << "List of integer sequences with links to LODA programs. An _Ln_ program is a LODA program of length _n_."
          << "\n\n";
      list_file << buf;
    }
  }
  Log::get().info( "Finished generation of stats for " + std::to_string( num_processed ) + " programs" );

}

size_t OeisMaintenance::checkPrograms()
{
  Log::get().info( "Checking correctness of OEIS programs" );
  size_t num_processed = 0, num_removed = 0;
  Parser parser;
  Program program;
  std::string file_name;
  bool is_okay;

  // check programs for all sequences
  for ( auto &s : manager.sequences )
  {
    if ( s.id == 0 )
    {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file( file_name );
    if ( program_file.good() )
    {
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Checking program for " + s.to_string() );
      }
      try
      {
        program = parser.parse( program_file );
        s.fetchBFile(); // ensure b-file is loaded
        auto very_long_seq = s.getTerms( OeisSequence::VERY_LONG_SEQ_LENGTH );

        // check its correctness
        auto check = interpreter.check( program, very_long_seq, OeisSequence::LONG_SEQ_LENGTH );
        is_okay = check.first;
      }
      catch ( const std::exception &exc )
      {
        Log::get().error( "Error checking " + file_name + ": " + std::string( exc.what() ), false );
        is_okay = false;
        continue;
      }

      if ( !is_okay )
      {
        Log::AlertDetails details;
        details.title = s.id_str();
        details.title_link = s.url_str();
        details.color = "danger";
        details.text = "Removing invalid program for " + s.to_string();
        Log::get().alert( details.text, details );
        program_file.close();
        remove( file_name.c_str() );
      }
      if ( ++num_processed % 100 == 0 )
      {
        Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
      }
    }
  }

  if ( num_removed > 0 )
  {
    Log::get().alert( "Removed " + std::to_string( num_removed ) + " invalid programs" );
  }
  return num_removed;
}

size_t OeisMaintenance::minimizePrograms()
{
  Log::get().info( "Minimizing OEIS programs" );
  size_t num_processed = 0, num_minimized = 0;
  Parser parser;
  Program program, minimized;
  Sequence result;
  std::string file_name;
  bool is_manual;
  steps_t steps;

  // minimize programs for all sequences
  for ( auto &s : manager.sequences )
  {
    if ( s.id == 0 )
    {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file( file_name );
    if ( program_file.good() )
    {
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Minimizing program for " + s.to_string() );
      }
      try
      {
        program = parser.parse( program_file );
      }
      catch ( const std::exception &exc )
      {
        Log::get().error( "Error parsing " + file_name + ": " + std::string( exc.what() ), false );
        continue;
      }

      is_manual = false;
      if ( program.ops.size() > 1 && program.ops[1].type == Operation::Type::NOP )
      {
        is_manual = program.ops[1].comment.find( "Coded manually" ) != std::string::npos;
      }
      if ( !is_manual )
      {
        ProgramUtil::removeOps( program, Operation::Type::NOP );
        minimized = program;
        manager.minimizer.optimizeAndMinimize( minimized, 2, 1, OeisSequence::LONG_SEQ_LENGTH );
        if ( program != minimized )
        {
          Log::get().info( "Updating program because it is not minimal: " + file_name );
          num_minimized++;
        }
        manager.dumpProgram( s.id, minimized, file_name );
      }

      if ( ++num_processed % 100 == 0 )
      {
        Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
      }
    }
  }

  if ( num_minimized > 0 )
  {
    Log::get().alert(
        "Optimized " + std::to_string( num_minimized ) + "/" + std::to_string( num_processed ) + " programs" );
  }
  return num_minimized;
}
