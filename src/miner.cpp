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

void Miner::Mine( volatile sig_atomic_t& exit_flag )
{
  Oeis oeis( settings );
  oeis.load();
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
      optimizer.optimize( program, 1 );
      if ( oeis.findSequence( program ) != id )
      {
        std::cout << "before:" << std::endl;
        Printer d;
        d.print( backup, std::cout );
        std::cout << "after:" << std::endl;
        d.print( program, std::cout );
        Log::get().error( "Program generates wrong result after minimization and optimization", true );
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
          if ( existing_program.num_ops( false ) > 0 && existing_program.num_ops( false ) <= program.num_ops( false ) )
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
