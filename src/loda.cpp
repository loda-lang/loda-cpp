#include "database.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "test.hpp"
#include "util.hpp"

#include <chrono>
#include <csignal>
#include <fstream>
#include <sstream>
#include <unordered_set>

void help()
{
  Settings settings;
  std::cout << "Usage:             loda <command> <options>" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  evaluate <file>  Evaluate a program to a sequence" << std::endl;
  std::cout << "  optimize <file>  Optimize a program and print it" << std::endl;
  std::cout << "  generate         Generate a random program and print it" << std::endl;
  std::cout << "  mine             Mine programs for OEIS sequences" << std::endl;
//  std::cout << "  insert           Insert a program into the database" << std::endl;
//  std::cout << "  search           Search a program in the database" << std::endl;
//  std::cout << "  programs         Print all program in the database" << std::endl;
//  std::cout << "  sequences        Print all sequences in the database" << std::endl;
  std::cout << "  test             Run test suite" << std::endl;
  std::cout << "  help             Print this help text" << std::endl;
  std::cout << "General options:" << std::endl;
  std::cout << "  -l <string>      Log level (values:debug,info,warn,error)" << std::endl;
  std::cout << "  -t <number>      Number of sequence terms (default:" << settings.num_terms << ")" << std::endl;
  std::cout << "Interpeter options:" << std::endl;
  std::cout << "  -c <number>      Maximum number of interpreter cycles (default:" << settings.max_cycles << ")"
      << std::endl;
  std::cout << "  -m <number>      Maximum number of used memory cells (default:" << settings.max_memory << ")" << std::endl;
  std::cout << "Generator options:" << std::endl;
  std::cout << "  -p <number>      Maximum number of operations (default:" << settings.num_operations << ")" << std::endl;
  std::cout << "  -n <number>      Maximum constant (default:" << settings.max_constant << ")" << std::endl;
  std::cout << "  -o <string>      Operation types (default:" << settings.operation_types << ";a:add,s:sub,m:mov,l:lpb/lpe)" << std::endl;
  std::cout << "  -a <string>      Operand types (default:" << settings.operand_types << ";c:constant,d:direct mem,i:indirect mem)" << std::endl;
  std::cout << "  -e <file>        Program template" << std::endl;
}

volatile sig_atomic_t EXIT_FLAG = 0;

void handleSignal( int s )
{
  EXIT_FLAG = 1;
}

int main( int argc, char *argv[] )
{
  std::signal( SIGINT, handleSignal );
  Settings settings;
  auto args = settings.parseArgs( argc, argv );
  if ( !args.empty() )
  {
    std::string cmd = args.front();
    if ( cmd == "help" )
    {
      help();
    }
    else if ( cmd == "test" )
    {
      Test test;
      test.all();
    }
    else if ( cmd == "evaluate" || cmd == "eval" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Interpreter interpreter( settings );
      auto sequence = interpreter.eval( program );
      std::cout << sequence << std::endl;
    }
    else if ( cmd == "optimize" || cmd == "opt" )
    {
      Parser parser;
      Printer printer;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Optimizer optimizer( settings );
      optimizer.optimize( program, 1 );
      printer.print( program, std::cout );
    }
    else if ( cmd == "insert" )
    {
      Parser a;
      Program p = a.parse( std::string( args.at( 1 ) ) );
      Database db( settings );
      db.insert( std::move( p ) );
      db.save();
    }
    else if ( cmd == "programs" )
    {
      Database db( settings );
      db.printPrograms();
    }
    else if ( cmd == "sequences" )
    {
      Database db( settings );
      db.printSequences();
    }
    else if ( cmd == "generate" )
    {
      Optimizer optimizer( settings );
      Generator generator( settings, 5, std::random_device()() );
      auto program = generator.generateProgram();
      optimizer.optimize( program, 1 );
      Printer r;
      r.print( program, std::cout );
    }
    else if ( cmd == "mine" )
    {
      Oeis oeis( settings );
      oeis.load();
      Optimizer optimizer( settings );
      Generator generator( settings, 5, std::random_device()() );
      size_t count = 0;
      auto time = std::chrono::steady_clock::now();
      while ( !EXIT_FLAG )
      {
        auto program = generator.generateProgram();
        optimizer.optimize( program, 1 );
        auto id = oeis.findSequence( program );
        if ( id )
        {
          optimizer.minimize( program, oeis.sequences[id].full.size() );
          optimizer.optimize( program, 1 );
          std::string file_name = "programs/oeis/" + oeis.sequences[id].id_str() + ".asm";
          bool write_file = true;
          {
            std::ifstream in( file_name );
            if ( in.good() )
            {
              Parser parser;
              auto existing_program = parser.parse( in );
              if ( existing_program.num_ops( false ) > 0
                  && existing_program.num_ops( false ) <= program.num_ops( false ) )
              {
                write_file = false;
              }
            }
          }
          if ( write_file )
          {
            std::stringstream buf;
            buf << "Found new program for " << oeis.sequences[id];
            Log::get().info( buf.str() );
            buf.str( "" );
            buf << "First " << oeis.sequences[id].size() << " terms of " << oeis.sequences[id].id_str() << ": "
                << static_cast<Sequence>( oeis.sequences[id] );
            Log::get().debug( buf.str() );

            oeis.dumpProgram( id, program, file_name );
          }
        }
        ++count;
        auto time2 = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>( time2 - time );
        if ( duration.count() >= 10 )
        {
          time = time2;
          Log::get().info( "Generated " + std::to_string( count ) + " programs" );
          generator.mutate( 0.5 );
          generator.setSeed( std::random_device()() );
        }
      }
    }
    else
    {
      std::cerr << "Unknown command: " << cmd << std::endl;
      return EXIT_FAILURE;
    }

  }
  else
  {
    help();
  }
  return EXIT_SUCCESS;
}

