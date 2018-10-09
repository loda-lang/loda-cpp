#include "test.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "iterator.hpp"
#include "number.hpp"
#include "oeis.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "serializer.hpp"

#include <fstream>

void Test::fibonacci()
{
  std::cout << "Running tests for programs/fibonacci.asm..." << std::endl;

  Sequence expected( { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
      17711, 28657, 46368, 75025 } );

  Parser parser;
  Settings settings;
  settings.num_terms = expected.size();
  Interpreter interpreter( settings );

  auto fib = parser.parse( "programs/fibonacci.asm" );
  auto result = interpreter.eval( fib );
  std::cout << "fib=" << result << "..." << std::endl;
  if ( result != expected )
  {
    throw std::runtime_error( "unexpected result" );
  }
  std::cout << std::endl;

}

void Test::exponentiation()
{
  std::vector<std::vector<number_t> > values = { { 1, 0, 0, 0 }, { 1, 1, 1, 1 }, { 1, 2, 4, 8 }, { 1, 3, 9, 27 }, { 1,
      4, 16, 64 } };
  testBinary( "exp", "programs/exponentiation.asm", values );
}

void Test::ackermann()
{
  std::vector<std::vector<number_t> > values = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 6 }, { 3, 5, 7, 9, 11 }, { 5, 13, 29,
      61, 125 }, { 13, 65533 } };
  testBinary( "ack", "programs/ackermann.asm", values );
}

void Test::find()
{
  Sequence expected( { 0, 1, 1, 2, 3, 5, 8, 13 } );
//  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
//    17711, 28657, 46368, 75025};

  Settings settings;
  settings.num_terms = expected.size();
  Finder finder( settings );
  FixedSequenceScorer scorer( expected );
  finder.find( scorer, 23, 10 );

}

void Test::iterate()
{
  Iterator it;
  Printer printer;
  while ( true )
  {
    std::cout << "\x1B[2J\x1B[H";
    printer.print( it.next(), std::cout );
//    std::cin.ignore();
  }
}

void Test::serialize()
{
  Parser parser;
  Printer printer;
//  Serializer serializer;
  auto fib = parser.parse( "programs/fibonacci.asm" );
  printer.print( fib, std::cout );
}

void Test::oeis()
{
  Settings settings;
  Oeis o( settings );
  o.load();
  for ( auto& s : o.sequences )
  {
    std::ifstream file( "programs/oeis/" + s.id_str() + ".asm" );
    if ( file.good() )
    {
      std::cout << "Generating first " << s.full.size() << " terms of sequence " << s << std::endl;
      Parser parser;
      Program program = parser.parse( file );
      Settings settings;
      settings.num_terms = s.full.size();
      Interpreter interpreter( settings );
      Sequence result = interpreter.eval( program );
      if ( result.size() != s.full.size() || result != s.full )
      {
        Log::get().error( "Program did not evaluate to expected sequence!", true );
      }
    }
  }
  std::cout << std::endl;
}

void Test::all()
{
//  Iterate();
//  Find();
  fibonacci();
  oeis();
  exponentiation();
  ackermann();
}

void Test::testBinary( const std::string& func, const std::string& file,
    const std::vector<std::vector<number_t> >& values )
{
  std::cout << "Running tests for " << file << "..." << std::endl;
  Parser parser;
  Settings settings;
  settings.max_cycles = 10000000; // needed to run ackermann test
  Interpreter interpreter( settings );
  auto program = parser.parse( file );
  for ( size_t i = 0; i < values.size(); i++ )
  {
    for ( size_t j = 0; j < values[i].size(); j++ )
    {
      std::cout << func << "(" << i << "," << j << ")=";
      Memory mem;
      mem.set( 0, i );
      mem.set( 1, j );
      interpreter.run( program, mem );
      std::cout << mem.get( 2 ) << std::endl;
      if ( mem.get( 2 ) != values[i][j] )
      {
        throw std::runtime_error( "unexpected result: " + std::to_string( mem.get( 2 ) ) );
      }
    }
  }
  std::cout << std::endl;
}
