#include "miner.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"
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
  oeis.load();
}

bool Miner::updateCollatz( const Program &p, const Sequence &seq ) const
{
  if ( isCollatzValuation( seq ) )
  {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch() );
    std::string file_name = "programs/oeis/collatz_" + std::to_string( ms.count() % 1000000 ) + ".asm";
    std::ofstream out( file_name );
    out << "; " << seq << std::endl;
    out << std::endl;
    Printer r;
    r.print( p, out );
    out.close();
    Log::get().alert( "Found possible Collatz valuation: " + seq.to_string() );
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
  for ( size_t i = 3; i < seq.size(); i++ )
  {
    if ( i % 2 == 0 ) // even
    {
      if ( seq[i / 2] >= seq[i] )
      {
        return false;
      }
    }
    else // odd
    {
      size_t j = 3 * i + 1;
      if ( j < seq.size() && seq[j] >= seq[i] )
      {
        return false;
      }
    }
  }
  return true;
}

void Miner::mine( volatile sig_atomic_t &exit_flag )
{
  Log::get().info( "Mining programs for OEIS sequences" );
  Generator generator( settings, 5, std::random_device()() );
  Sequence norm_seq;
  size_t count = 0;
  size_t found = 0;
  auto time = std::chrono::steady_clock::now();

  while ( !exit_flag )
  {
    auto program = generator.generateProgram();
    auto seq_programs = oeis.findSequence( program, norm_seq );
    for ( auto s : seq_programs )
    {
      if ( oeis.updateProgram( s.first, s.second ) )
      {
        ++found;
      }
    }
    if ( updateCollatz( program, norm_seq ) )
    {
      ++found;
    }
    ++count;
    auto time2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>( time2 - time );
    if ( duration.count() >= 60 )
    {
      time = time2;
      Log::get().info( "Generated " + std::to_string( count ) + " programs" );
      if ( found == 0 )
      {
        generator.mutate( 0.5 );
        generator.setSeed( std::random_device()() );
      }
      else
      {
        --found;
      }
    }
  }
}

void Miner::synthesize( volatile sig_atomic_t &exit_flag )
{
  Log::get().info( "Start synthesizing programs for OEIS sequences" );
  std::vector<std::unique_ptr<Synthesizer>> synthesizers;
  synthesizers.resize( 1 );
  synthesizers[0].reset( new LinearSynthesizer() );
  Program program;
  size_t found = 0;
  for ( auto &synthesizer : synthesizers )
  {
    for ( auto &seq : oeis.getSequences() )
    {
      if ( synthesizer->synthesize( seq.full, program ) )
      {
        Log::get().debug( "Synthesized program for " + seq.to_string() );
        oeis.updateProgram( seq.id, program );
      }
      if ( exit_flag )
      {
        return;
      }
    }
  }
  Log::get().info( "Finished synthesizing " + std::to_string( found ) + " programs" );
}
