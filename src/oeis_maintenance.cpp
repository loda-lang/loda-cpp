#include "oeis_maintenance.hpp"

#include "oeis_list.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "stats.hpp"
#include "util.hpp"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <random>
#include <sstream>

OeisMaintenance::OeisMaintenance( const Settings &settings )
    : evaluator( settings ),
      minimizer( settings ),
      manager( settings, true ) // we need to set the overwrite-flag here!
{
}

void OeisMaintenance::maintain()
{
  // load sequence data
  manager.load();

  // update stats
  manager.getStats();

  // generate lists
  generateLists();

  // check and minimize programs
  auto num_changed = checkAndMinimizePrograms();

  // generate lists again if there was a change
  if ( num_changed > 0 )
  {
    generateLists();
  }
  Log::get().info( "Finished maintenance of programs" );
}

void OeisMaintenance::generateLists()
{
  manager.load();
  Log::get().info( "Generating program lists" );
  const size_t list_file_size = 50000;
  std::vector<std::stringstream> list_files( 1000000 / list_file_size );
  std::stringstream no_loda;
  size_t num_processed = 0;
  Parser parser;
  Program program;
  std::string file_name;

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
      try
      {
        program = parser.parse( program_file );
      }
      catch ( const std::exception &exc )
      {
        Log::get().error( "Error parsing " + file_name + ": " + std::string( exc.what() ), false );
        continue;
      }

      // update program list
      size_t list_index = (s.id + 1) / list_file_size;
      list_files.at( list_index ) << "* [" << s.id_str() << "](https://oeis.org/" << s.id_str()
          << ") ([program](/edit/?oeis=" << s.id << ")): " << s.name << "\n";

      num_processed++;

      //if ( num_processed % 1000 == 0 )
      //{
      //  Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
      //}
    }
    else
    {
      no_loda << s.id_str() << ": " << s.name << "\n";
    }
  }

  // write lists
  const std::string lists_home = "../loda-lang.github.io/";
  ensureDir( lists_home );
  for ( size_t i = 0; i < list_files.size(); i++ )
  {
    auto buf = list_files[i].str();
    if ( !buf.empty() )
    {
      const std::string list_path = lists_home + "list" + std::to_string( i ) + ".markdown";
      OeisSequence start( std::max<int64_t>( i * list_file_size, 1 ) );
      OeisSequence end( ((i + 1) * list_file_size) - 1 );
      std::ofstream list_file( list_path );
      list_file << "---\n";
      list_file << "layout: page\n";
      list_file << "title: List " << i << "\n";
      list_file << "permalink: /list " << i << "/\n";
      list_file << "---\n";
      list_file << "# Programs for " << start.id_str() << "-" << end.id_str() << "\n\n";
      list_file
          << "List of integer sequences with links to LODA programs."
          << "\n\n";
      list_file << buf;
    }
  }
  std::ofstream no_loda_file( OeisList::getListsHome() + "no_loda.txt" );
  no_loda_file << no_loda.str();
  no_loda_file.close();

  // publish metrics
  auto& stats = manager.getStats();
  std::vector<Metrics::Entry> entries;
  std::map<std::string, std::string> labels;

  labels["kind"] = "total";
  entries.push_back( { "programs", labels, (double) stats.num_programs } );
  entries.push_back( { "sequences", labels, (double) manager.getTotalCount() } );

  labels["kind"] = "used";
  entries.push_back( { "sequences", labels, (double) stats.num_sequences } );

  labels.clear();
  for ( size_t i = 0; i < stats.num_ops_per_type.size(); i++ )
  {
    if ( stats.num_ops_per_type[i] > 0 )
    {
      labels["type"] = Operation::Metadata::get( static_cast<Operation::Type>( i ) ).name;
      entries.push_back( { "operation_types", labels, (double) stats.num_ops_per_type[i] } );
    }
  }
  Metrics::get().write( entries );

  Log::get().info( "Finished generation of lists for " + std::to_string( num_processed ) + " programs" );
}

size_t OeisMaintenance::checkAndMinimizePrograms()
{
  Log::get().info( "Checking and minimizing programs" );
  size_t num_processed = 0, num_removed = 0, num_minimized = 0;
  Parser parser;
  Program program, minimized;
  std::string file_name;
  bool is_okay, is_protected, is_manual;

  // generate random order of sequences
  std::vector<size_t> ids;
  ids.resize( manager.sequences.size() );
  const size_t l = ids.size();
  for ( size_t i = 0; i < l; i++ )
  {
    ids[i] = i;
  }
  auto rng = std::default_random_engine { };
  rng.seed( std::chrono::system_clock::now().time_since_epoch().count() );
  std::shuffle( std::begin( ids ), std::end( ids ), rng );

  // check programs for all sequences
  for ( auto id : ids )
  {
    auto& s = manager.sequences[id];
    if ( s.id == 0 )
    {
      continue;
    }
    file_name = s.getProgramPath();
    std::ifstream program_file( file_name );
    if ( program_file.good() )
    {
      Log::get().debug( "Checking program for " + s.to_string() );
      try
      {
        program = parser.parse( program_file );
        auto extended_seq = s.getTerms( OeisSequence::EXTENDED_SEQ_LENGTH );

        // check its correctness
        auto check = evaluator.check( program, extended_seq, OeisSequence::DEFAULT_SEQ_LENGTH, id );
        is_okay = (check.first != status_t::ERROR); // we allow warnings

        // check if it is on the deny list
        if ( manager.deny_list.find( s.id ) != manager.deny_list.end() )
        {
          is_okay = false;
        }
      }
      catch ( const std::exception &exc )
      {
        std::string what( exc.what() );
        if ( what.rfind( "Error fetching", 0 ) == 0 ) // user probably hit ctrl-c => exit
        {
          break;
        }
        Log::get().error( "Error checking " + file_name + ": " + std::string( exc.what() ), false );
        is_okay = false;
        continue;
      }

      if ( !is_okay )
      {
        // send alert
        manager.alert( program, id, "Removed invalid", "danger" );
        program_file.close();
        remove( file_name.c_str() );
      }
      else
      {
        is_protected = false;
        is_manual = false;
        if ( manager.protect_list.find( s.id ) != manager.protect_list.end() )
        {
          is_protected = true;
        }
        for ( const auto& op : program.ops )
        {
          if ( op.type == Operation::Type::NOP && op.comment.find( "Coded manually" ) != std::string::npos )
          {
            is_manual = true;
            break;
          }
        }
        if ( !is_protected && !is_manual )
        {
          ProgramUtil::removeOps( program, Operation::Type::NOP );
          minimized = program;
          minimizer.optimizeAndMinimize( minimized, 2, 1, OeisSequence::DEFAULT_SEQ_LENGTH );
          if ( program != minimized )
          {
            // manager.alert( minimized, id, "Minimized", "warning" );
            num_minimized++;
          }
          manager.dumpProgram( s.id, minimized, file_name );
        }
      }

//      if ( ++num_processed % 100 == 0 )
//      {
//        Log::get().info( "Processed " + std::to_string( num_processed ) + " programs" );
//      }
    }
  }

  if ( num_removed > 0 )
  {
    Log::get().info( "Removed " + std::to_string( num_removed ) + " invalid programs" );
  }
  if ( num_minimized > 0 )
  {
    Log::get().info(
        "Minimized " + std::to_string( num_minimized ) + "/" + std::to_string( num_processed ) + " programs" );
  }

  return num_removed + num_minimized;
}
