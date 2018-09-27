#include "database.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "test.hpp"
#include "util.hpp"

#include <csignal>

void help()
{
  std::cout << "Usage: loda <cmd>" << std::endl;
  std::cout << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  help       Print this help text" << std::endl;
  std::cout << "  test       Run test suite" << std::endl;
  std::cout << "  insert     Insert a program into the database" << std::endl;
  std::cout << "  generate   Generate random programs and store them in the database" << std::endl;
  std::cout << "  search     Search a program in the database" << std::endl;
  std::cout << "  programs   Print all program in the database" << std::endl;
  std::cout << "  sequences  Print all sequences in the database" << std::endl;
}

volatile sig_atomic_t EXIT_FLAG = 0;

void handleSignal( int s )
{
  EXIT_FLAG = 1;
}

int main( int argc, char *argv[] )
{
  std::signal( SIGINT, handleSignal );
  if ( argc > 1 )
  {
    std::string cmd( argv[1] );
    if ( cmd == "help" )
    {
      help();
    }
    else if ( cmd == "test" )
    {
      Test test;
      test.all();
    }
    else if ( cmd == "insert" )
    {
      Parser a;
      Program p = a.parse( std::string( argv[2] ) );
      Database db;
      db.insert( std::move( p ) );
      db.save();
    }
    else if ( cmd == "programs" )
    {
      Database db;
      db.printPrograms();
    }
    else if ( cmd == "sequences" )
    {
      Database db;
      db.printSequences();
    }
    else if ( cmd == "generate" )
    {
      size_t length = 32;
      Oeis oeis( length );
      oeis.load();
      Database db;
      //Finder f;
      Generator g( 5, std::random_device()() );
      Interpreter t;
      size_t count = 0;
      while ( !EXIT_FLAG )
      {
        //auto p = f.find( oeis, length, std::random_device()(), 20 );
        auto p = g.generateProgram();
        try
        {
          auto s = t.eval( p, length );
          if ( oeis.score( s ) == 0 )
          {
            Printer r;
            r.print( p, std::cout );
            db.insert( std::move( p ) );
          }
        }
        catch ( const std::exception& )
        {
        }
        if ( ++count % 100000 == 0 )
        {
          Log::get().info( "Generated " + std::to_string( count ) + " programs" );
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

