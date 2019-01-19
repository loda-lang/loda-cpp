#include "miner.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_set>

void NotifyUnexpectedResult( const Program& before, const Program& after, const std::string& process)
{
  std::cout << "before:" << std::endl;
  Printer d;
  d.print( before, std::cout );
  std::cout << "after:" << std::endl;
  d.print( after, std::cout );
  Log::get().error( "Program generates wrong result after " + process, true );
}

void Miner::Mine( volatile sig_atomic_t& exit_flag )
{
  Optimizer optimizer( settings );
  Generator generator( settings, 5, std::random_device()() );
  size_t count = 0;
  size_t found = 0;
  auto time = std::chrono::steady_clock::now();
  while ( !exit_flag )
  {
    auto program = generator.generateProgram();
    auto id = oeis.findSequence( program );
    if ( id )
    {
      Program backup = program;
      optimizer.minimize( program, oeis.sequences[id].full.size() );
      if ( oeis.findSequence( program ) != id )
      {
        NotifyUnexpectedResult( backup, program, "minimization" );
      }
      backup = program;
      optimizer.optimize( program, 2, 1 );
      if ( oeis.findSequence( program ) != id )
      {
        NotifyUnexpectedResult( backup, program, "optimization" );
      }
      std::string file_name = "programs/oeis/" + oeis.sequences[id].id_str() + ".asm";
      bool write_file = true;
      bool is_new = true;
      {
        std::ifstream in( file_name );
        if ( in.good() )
        {
          is_new = false;
          Parser parser;
          auto existing_program = parser.parse( in );
          if ( existing_program.num_ops( false ) <= program.num_ops( false ) )
          {
            write_file = false;
          }
        }
      }
      if ( write_file )
      {
        std::stringstream buf;
        buf << "Found ";
        if ( is_new ) buf << "first";
        else buf << "shorter";
        buf << " program for " << oeis.sequences[id] << " First terms: " << static_cast<Sequence>( oeis.sequences[id] );
        Log::get().alert( buf.str() );
        oeis.dumpProgram( id, program, file_name );
        ++found;
      }
    }

    // CheckBounds( program );

    ++count;
    auto time2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast < std::chrono::seconds > (time2 - time);
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

double Miner::GetDiff( const Sequence& s1, const Sequence& s2 )
{
  size_t length = std::min( s1.size(), s2.size() );
  double diff = 0;
  for ( size_t i = 0; i < length; i++ )
  {
    int64_t d = ((int64_t) s2[i]) - ((int64_t) s1[i]);
    if ( d < 0 )
    {
      return -1;
    }
    diff += d;
  }
  return diff / (double) length;
}

double Miner::GetBounds( number_t id, bool upper, const OeisSequence& target_seq, bool force_update )
{
  if ( bounds.find( id ) == bounds.end() || force_update )
  {
    std::string file_name = "programs/oeis/" + oeis.sequences[id].id_str( upper ? "U" : "L" ) + ".asm";
    std::ifstream in( file_name );
    if ( in.good() )
    {
      Parser parser;
      auto existing_program = parser.parse( in );
      Interpreter interpreter( settings );
      try
      {
        auto seq = interpreter.eval( existing_program, target_seq.full.size() );
        double l_diff = GetDiff( seq, target_seq.full );
        double u_diff = GetDiff( target_seq.full, seq );
        bounds[id] = std::pair<double, double>( l_diff, u_diff );
      }
      catch ( ... )
      {
        bounds[id] = std::make_pair<double, double>( -1, -1 );
      }
    }
    else
    {
      bounds[id] = std::make_pair<double, double>( -1, -1 );
    }
  }
  return upper ? bounds[id].second : bounds[id].first;
}

void Miner::CheckBounds( const Program& p )
{
  std::vector<number_t> ids = { 40, 6577 };
  for ( number_t id : ids )
  {
    OeisSequence target_seq = oeis.sequences.at( id );
    Interpreter interpreter( settings );
    Sequence seq;
    try
    {
      seq = interpreter.eval( p, target_seq.full.size() );
    }
    catch ( ... )
    {
      return;
    }
    double l_diff = GetDiff( seq, target_seq.full );
    double u_diff = GetDiff( target_seq.full, seq );
    Log::get().debug( "Bounds: " + std::to_string( l_diff ) + " " + std::to_string( u_diff ) );
    if ( l_diff >= 0 )
    {
      double current_l_diff = GetBounds( id, false, target_seq, false );
      Log::get().debug( "Current lower bound: " + std::to_string( current_l_diff ) );
      if ( l_diff < current_l_diff || current_l_diff < 0 )
      {
        current_l_diff = GetBounds( id, false, target_seq, true );
        if ( l_diff < current_l_diff || current_l_diff < 0 )
        {
          UpdateBoundProgram( id, false, p, l_diff );
          bounds.erase( id );
        }
      }
    }
    if ( u_diff >= 0 )
    {
      double current_u_diff = GetBounds( id, true, target_seq, false );
      if ( u_diff < current_u_diff || current_u_diff < 0 )
      {
        current_u_diff = GetBounds( id, true, target_seq, true );
        if ( u_diff < current_u_diff || current_u_diff < 0 )
        {
          UpdateBoundProgram( id, true, p, u_diff );
          bounds.erase( id );
        }
      }
    }
  }
}

void Miner::UpdateBoundProgram( number_t id, bool upper, Program p, double bound )
{
  Optimizer optimizer( settings );
  optimizer.minimize( p, oeis.sequences[id].full.size() );
  optimizer.optimize( p, 2, 1 );

  std::stringstream buf;
  buf << "Found new ";
  if ( upper ) buf << "upper";
  else buf << "lower";
  buf << " bound for " << oeis.sequences[id] << " Average difference: " << bound;
  Log::get().alert( buf.str() );

  p.removeOps( Operation::Type::NOP );
  std::string file_name = "programs/oeis/" + oeis.sequences[id].id_str( upper ? "U" : "L" ) + ".asm";
  std::ofstream out( file_name );
  auto& seq = oeis.sequences[id];
  out << "; ";
  if ( upper ) out << "Upper";
  else out << "Lower";
  out << " bound for " << seq << std::endl;
  out << "; Average difference: " << bound << std::endl << std::endl;
  Printer r;
  r.print( p, out );

}

