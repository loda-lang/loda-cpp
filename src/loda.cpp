#include "database.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "test.hpp"
#include "util.hpp"

#include <csignal>
#include <fstream>

void help()
{
  Settings settings;
  std::cout << "Usage:       loda <command> <options>" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  eval       Evaluate a program to a sequence" << std::endl;
  std::cout << "  insert     Insert a program into the database" << std::endl;
  std::cout << "  generate   Generate random programs and store them in the database" << std::endl;
  std::cout << "  search     Search a program in the database" << std::endl;
  std::cout << "  programs   Print all program in the database" << std::endl;
  std::cout << "  sequences  Print all sequences in the database" << std::endl;
  std::cout << "  test       Run test suite" << std::endl;
  std::cout << "  help       Print this help text" << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -t         Number of sequence terms (default: " << settings.num_terms << ")" << std::endl;
  std::cout << "  -o         Number of operations per generated program (default: " << settings.num_operations << ")"
      << std::endl;
  std::cout << "  -m         Maximum number of memory cells (default: " << settings.max_memory << ")" << std::endl;
  std::cout << "  -c         Maximum number of interpreter cycles (default: " << settings.max_cycles << ")"
      << std::endl;
  std::cout << "  -l         Log level (values: debug, info, warn, error)" << std::endl;
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
    else if ( cmd == "eval" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Interpreter interpreter( settings );
      auto sequence = interpreter.eval( program );
      std::cout << sequence << std::endl;
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
      Oeis oeis( settings );
      oeis.load();
      Database db( settings );
      Generator generator( settings, 5, std::random_device()() );
      Interpreter t( settings );
      size_t count = 0;
      while ( !EXIT_FLAG )
      {
        //auto p = f.find( oeis, length, std::random_device()(), 20 );
        auto p = generator.generateProgram();
        try
        {
          auto s = t.eval( p );
          if ( oeis.score( s ) == 0 )
          {
            db.insert( std::move( p ) );
            auto id = oeis.ids[s];
            std::ofstream out( "programs/oeis/" + oeis.sequences[id].id_str() + ".asm" );
            out << "; " << oeis.sequences[id] << std::endl;
            out << "; " << s << std::endl << std::endl;
            Printer r;
            r.print( p, out );
          }
        }
        catch ( const std::exception& )
        {
        }
        if ( ++count % 1000 == 0 )
        {
          generator.mutate( 0.5 );
        }
        if ( ++count % 10000 == 0 )
        {
          Log::get().info( "Generated " + std::to_string( count ) + " programs" );
          generator.setSeed( std::random_device()() );
        }
      }
      db.save();
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

