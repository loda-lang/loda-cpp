#include "database.hpp"
#include "generator.hpp"
#include "interpreter.hpp"
#include "miner.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "test.hpp"
#include "util.hpp"

#include <csignal>

void help()
{
  Settings settings;
  std::cout << "Usage:             loda <command> <options>" << std::endl;
  std::cout << "Commands:" << std::endl;
  std::cout << "  evaluate <file>  Evaluate a program to a sequence" << std::endl;
  std::cout << "  optimize <file>  Optimize a program and print it" << std::endl;
  std::cout << "  minimize <file>  Minimize a program and print it (use -t to set the number of terms)" << std::endl;
  std::cout << "  generate         Generate a random program and print it" << std::endl;
  std::cout << "  mine             Mine programs for OEIS sequences" << std::endl;
//  std::cout << "  insert           Insert a program into the database" << std::endl;
//  std::cout << "  search           Search a program in the database" << std::endl;
//  std::cout << "  programs         Print all program in the database" << std::endl;
//  std::cout << "  sequences        Print all sequences in the database" << std::endl;
  std::cout << "  test             Run test suite" << std::endl;
  std::cout << "  help             Print this help text" << std::endl;
  std::cout << "General options:" << std::endl;
  std::cout << "  -l <string>      Log level (values:debug,info,warn,error,alert)" << std::endl;
  std::cout << "  -t <number>      Number of sequence terms (default:" << settings.num_terms << ")" << std::endl;
  std::cout << "Interpeter options:" << std::endl;
  std::cout << "  -c <number>      Maximum number of interpreter cycles (default:" << settings.max_cycles << ")"
      << std::endl;
  std::cout << "  -m <number>      Maximum number of used memory cells (default:" << settings.max_memory << ")"
      << std::endl;
  std::cout << "Generator options:" << std::endl;
  std::cout << "  -p <number>      Maximum number of operations (default:" << settings.num_operations << ")"
      << std::endl;
  std::cout << "  -n <number>      Maximum constant (default:" << settings.max_constant << ")" << std::endl;
  std::cout << "  -i <number>      Maximum index (default:" << settings.max_index << ")" << std::endl;
  std::cout << "  -x               Optimize and overwrite existing programs" << std::endl;
  std::cout << "  -o <string>      Operation types (default:" << settings.operation_types
      << ";a:add,s:sub,m:mov,l:lpb/lpe)" << std::endl;
  std::cout << "  -a <string>      Operand types (default:" << settings.operand_types
      << ";c:constant,d:direct mem,i:indirect mem)" << std::endl;
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
      optimizer.optimize( program, 2, 1 );
      printer.print( program, std::cout );
    }
    else if ( cmd == "minimize" || cmd == "min" )
    {
      Parser parser;
      Printer printer;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Optimizer optimizer( settings );
      optimizer.minimize( program, settings.num_terms );
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
      optimizer.optimize( program, 2, 1 );
      Printer r;
      r.print( program, std::cout );
    }
    else if ( cmd == "mine" )
    {
      Miner miner( settings );
      miner.Mine( EXIT_FLAG );
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

