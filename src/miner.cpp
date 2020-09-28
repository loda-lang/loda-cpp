#include "miner.hpp"

#include "generator_v1.hpp"
#include "interpreter.hpp"
#include "mutator.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "synthesizer.hpp"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_set>

Miner::Miner( const Settings &settings )
    :
    settings( settings ),
    oeis( settings ),
    interpreter( settings )
{
}

Generator::UPtr Miner::createGenerator( int64_t seed ) const
{
  Generator::UPtr generator;
  switch ( settings.generator_version )
  {
  case 1:
    generator.reset( new GeneratorV1( settings, seed ) );
    break;
  default:
    Log::get().error( "Invalid generator version: " + std::to_string( settings.generator_version ), true );
    break;
  }
  return generator;
}

bool Miner::updateSpecialSequences( const Program &p, const Sequence &seq ) const
{
  std::string kind;
  if ( isCollatzValuation( seq ) )
  {
    kind = "collatz";
  }
  if ( isPrimeSequence( seq ) )
  {
    kind = "primes";
  }
  if ( !kind.empty() )
  {
    std::string file_name = "programs/special/" + kind + "_" + std::to_string( ProgramUtil::hash( p ) % 1000000 )
        + ".asm";
    std::ofstream out( file_name );
    out << "; " << seq << std::endl;
    out << std::endl;
    ProgramUtil::print( p, out );
    out.close();
    Log::get().alert( "Found possible " + kind + " sequence: " + seq.to_string() );
    return true;

  }
  return false;
}

bool Miner::isCollatzValuation( const Sequence &seq )
{
  if ( seq.size() < 10 )
  {
    return false;
  }
  for ( size_t i = 1; i < seq.size() - 1; i++ )
  {
    int n = i + 1;
    if ( n % 2 == 0 ) // even
    {
      size_t j = (n / 2) - 1;
      if ( seq[j] >= seq[i] )
      {
        return false;
      }
    }
    else // odd
    {
      size_t j = (((3 * n) + 1) / 2) - 1;
      if ( j < seq.size() && seq[j] >= seq[i] )
      {
        return false;
      }
    }
  }
  return true;
}

bool Miner::isPrimeSequence( const Sequence &seq ) const
{
  if ( seq.size() < 10 )
  {
    return false;
  }
  if ( primes_cache.empty() )
  {
    Log::get().debug( "Loading prime numbers" );
    auto &primes = oeis.getSequences().at( 40 );
    if ( primes.full.at( 10 ) != 31 )
    {
      Log::get().error(
          "Expected 10th value of primes (A000040) to be 31, but found " + std::to_string( primes.full.at( 10 ) ),
          false );
    }
    primes_cache.insert( primes.begin(), primes.end() );
  }
  std::unordered_set<number_t> found;
  for ( auto n : seq )
  {
    if ( primes_cache.count( n ) == 0 )
    {
      return false;
    }
    if ( found.count( n ) > 0 )
    {
      return false;
    }
    found.insert( n );
  }
  return true;
}

void Miner::mine( volatile sig_atomic_t &exit_flag )
{
  oeis.load( exit_flag );
  Log::get().info( "Mining programs for OEIS sequences" );

  // metrics labels
  std::map<std::string, std::string> settings_labels = { { "generator_version", std::to_string(
      settings.generator_version ) }, { "num_operations", std::to_string( settings.num_operations ) }, { "max_constant",
      std::to_string( settings.max_constant ) }, { "max_index", std::to_string( settings.max_index ) }, {
      "operation_types", settings.operation_types }, { "operand_types", settings.operand_types }, };
  if ( !settings.program_template.empty() )
  {
    settings_labels.emplace( "program_template", settings.program_template );
  }

  auto &finder = oeis.getFinder();

  std::random_device rand;
  auto generator = createGenerator( rand() );
  Mutator mutator( rand() );
  std::stack<Program> progs;
  Sequence norm_seq;
  size_t generated = 0;
  size_t fresh = 0;
  size_t updated = 0;
  auto time = std::chrono::steady_clock::now();

  while ( !exit_flag )
  {
    if ( progs.empty() )
    {
      progs.push( generator->generateProgram() );
    }
    Program program = progs.top();
    progs.pop();
    auto seq_programs = finder.findSequence( program, norm_seq, oeis.getSequences() );
    for ( auto s : seq_programs )
    {
      auto r = oeis.updateProgram( s.first, s.second );
      if ( r.first )
      {
        if ( r.second )
        {
          fresh++;
        }
        else
        {
          updated++;
        }
        if ( progs.size() < 1000 || settings.hasMemory() )
        {
          mutator.mutateConstants( s.second, 100, progs );
        }
      }
    }
    if ( updateSpecialSequences( program, norm_seq ) )
    {
      fresh++;
    }
    generated++;
    auto time2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>( time2 - time );
    if ( duration.count() >= 60 )
    {
      time = time2;
      Log::get().info( "Generated " + std::to_string( generated ) + " programs" );
      Metrics::get().write( "generated", settings_labels, generated );
      Metrics::get().write( "fresh", settings_labels, fresh );
      Metrics::get().write( "updated", settings_labels, updated );
      generated = 0;
      fresh = 0;
      updated = 0;
      finder.publishMetrics();
    }
  }
}

void Miner::synthesize( volatile sig_atomic_t &exit_flag )
{
  Log::get().info( "Start synthesizing programs for OEIS sequences" );
  bool tweet_alerts = Log::get().tweet_alerts;
  Log::get().tweet_alerts = false;
  std::vector<std::unique_ptr<Synthesizer>> synthesizers;
  synthesizers.resize( 2 );
  synthesizers[0].reset( new LinearSynthesizer() );
  synthesizers[1].reset( new PeriodicSynthesizer() );
  Program program;
  size_t found = 0;
  auto &finder = oeis.getFinder();
  for ( auto &synthesizer : synthesizers )
  {
    for ( auto &seq : oeis.getSequences() )
    {
      if ( exit_flag )
      {
        break;
      }
      if ( seq.empty() )
      {
        continue;
      }
      if ( synthesizer->synthesize( seq.full, program ) )
      {
        Log::get().debug( "Synthesized program for " + seq.to_string() );
        auto r = oeis.updateProgram( seq.id, program );
        if ( r.first )
        {
          found++;
        }
      }
    }
    for ( auto &matcher : finder.getMatchers() )
    {
      for ( auto &reduced : matcher->getReducedSequences() )
      {
        if ( exit_flag )
        {
          break;
        }
        if ( reduced.first.empty() )
        {
          continue;
        }
        if ( synthesizer->synthesize( reduced.first, program ) )
        {
          Sequence seq;
          auto progs = finder.findSequence( program, seq, oeis.getSequences() );
          for ( auto &p : progs )
          {
            Log::get().debug( "Synthesized program for " + OeisSequence( p.first ).to_string() );
            auto r = oeis.updateProgram( p.first, p.second );
            if ( r.first )
            {
              found++;
            }
          }
        }
      }
    }
  }
  Log::get().tweet_alerts = tweet_alerts;
  if ( found > 0 )
  {
    Log::get().alert( "Synthesized " + std::to_string( found ) + " new or shorter programs" );
  }
  else
  {
    Log::get().info( "Finished synthesis without new results" );
  }
}
