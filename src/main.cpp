#include "interpreter.hpp"
#include "miner.hpp"
#include "minimizer.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "program_util.hpp"
#include "test.hpp"
#include "util.hpp"

#include <csignal>
#include <execinfo.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void help()
{
  std::stringstream operation_types;
  bool is_first = true;
  for ( auto &t : Operation::Types )
  {
    auto &m = Operation::Metadata::get( t );
    if ( m.is_public && t != Operation::Type::LPE )
    {
      if ( !is_first ) operation_types << ",";
      operation_types << std::string( 1, m.short_name ) << ":" << m.name;
      is_first = false;
    }
  }
  Settings settings;
  std::cout << "Usage:             loda <command> <options>" << std::endl;
  std::cout << "Core commands:" << std::endl;
  std::cout << "  evaluate <file>  Evaluate a program to a sequence" << std::endl;
  std::cout << "  optimize <file>  Optimize a program and print it" << std::endl;
  std::cout << "  minimize <file>  Minimize a program and print it (use -t to set the number of terms)" << std::endl;
  std::cout << "  optmin <file>    Optimize and minimize a program and print it (use -t to set the number of terms)"
      << std::endl;
  std::cout << "  generate         Generate a random program and print it" << std::endl;
  std::cout << "  test             Run test suite" << std::endl;
  std::cout << "OEIS commands:" << std::endl;
  std::cout << "  mine             Mine programs for OEIS sequences" << std::endl;
  std::cout << "  synthesize       Synthesize programs for OEIS sequences" << std::endl;
  std::cout << "  maintain         Maintain programs for OEIS sequences" << std::endl;
  std::cout << "  update           Update OEIS index" << std::endl;
  std::cout << "General options:" << std::endl;
  std::cout << "  -l <string>      Log level (values:debug,info,warn,error,alert)" << std::endl;
  std::cout << "  -k <string>      Configuration file (default:loda.json)" << std::endl;
  std::cout << "  -t <number>      Number of sequence terms (default:" << settings.num_terms << ")" << std::endl;
  std::cout << "  -s <number>      Maximum physical memory (default:" << settings.max_physical_memory / (1024 * 1024)
      << ")" << std::endl;
  std::cout << "Interpreter options:" << std::endl;
  std::cout << "  -c <number>      Maximum number of interpreter cycles (default:" << settings.max_cycles << ")"
      << std::endl;
  std::cout << "  -m <number>      Maximum number of used memory cells (default:" << settings.max_memory << ")"
      << std::endl;
  std::cout << "Miner options:" << std::endl;
  std::cout << "  -r               Search for programs of linear sequences (slow)" << std::endl;
  std::cout << "  -x               Optimize and overwrite existing programs" << std::endl;
}

volatile sig_atomic_t EXIT_FLAG = 0;

void handle_sigint( int s )
{
  if ( !EXIT_FLAG )
  {
    Log::get().info( "Shutting down instance" );
    EXIT_FLAG = 1;
  }
}

void handle_sigsegv( int s )
{
  void *array[10];
  size_t size;
  size = backtrace( array, 10 );
  fprintf( stderr, "Fatal: signal %d:\n", s );
  backtrace_symbols_fd( array, size, STDERR_FILENO );
  exit( 1 );
}

int main( int argc, char *argv[] )
{
  std::signal( SIGINT, handle_sigint );
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
      Test test( EXIT_FLAG );
      test.all();
    }
    else if ( cmd == "evaluate" || cmd == "eval" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Interpreter interpreter( settings );
      Sequence seq;
      interpreter.eval( program, seq );
      std::cout << seq << std::endl;
    }
    else if ( cmd == "optimize" || cmd == "opt" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Optimizer optimizer( settings );
      optimizer.optimize( program, 2, 1 );
      ProgramUtil::print( program, std::cout );
    }
    else if ( cmd == "minimize" || cmd == "min" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Minimizer minimizer( settings );
      minimizer.minimize( program, settings.num_terms );
      ProgramUtil::print( program, std::cout );
    }
    else if ( cmd == "optmin" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Minimizer minimizer( settings );
      minimizer.optimizeAndMinimize( program, 2, 1, settings.num_terms );
      ProgramUtil::print( program, std::cout );
    }
    else if ( cmd == "generate" || cmd == "gen" )
    {
      Optimizer optimizer( settings );
      std::random_device rand;
      MultiGenerator multi_generator( settings, rand() );
      auto program = multi_generator.getGenerator()->generateProgram();
      ProgramUtil::print( program, std::cout );
    }
    else if ( cmd == "mine" )
    {
      Miner miner( settings );
      miner.mine( EXIT_FLAG );
    }
    else if ( cmd == "synthesize" || cmd == "syn" )
    {
      Miner miner( settings );
      miner.synthesize( EXIT_FLAG );
    }
    else if ( cmd == "maintain" )
    {
      // need to set the override flag!
      settings.optimize_existing_programs = true;
      Oeis o( settings );
      o.maintain( EXIT_FLAG );
    }
    else if ( cmd == "update" )
    {
      // need to set the override flag!
      settings.optimize_existing_programs = true;
      Oeis o( settings );
      o.update( EXIT_FLAG );
    }
    else if ( cmd == "migrate" )
    {
      Oeis o( settings );
      o.migrate( EXIT_FLAG );
    }
    else if ( cmd == "collatz" )
    {
      Parser parser;
      Program program = parser.parse( std::string( args.at( 1 ) ) );
      Interpreter interpreter( settings );
      Sequence seq;
      interpreter.eval( program, seq );
      bool is_collatz = Miner::isCollatzValuation( seq );
      std::cout << (is_collatz ? "true" : "false") << std::endl;
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

