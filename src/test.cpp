#include "test.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "iterator.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "serializer.hpp"

void Test::fibonacci()
{
  std::cout << "Running tests for examples/fibonacci.asm..." << std::endl;

  Sequence expected( { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
      17711, 28657, 46368, 75025 } );

  Parser parser;
  Interpreter interpreter;

  auto fib = parser.parse( "examples/fibonacci.asm" );
  auto result = interpreter.eval( fib, expected.size() );
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
  testBinary( "exp", "examples/exponentiation.asm", values );
}

void Test::ackermann()
{
  std::vector<std::vector<number_t> > values = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 6 }, { 3, 5, 7, 9, 11 }, { 5, 13, 29,
      61, 125 }, { 13, 65533 } };
  testBinary( "ack", "examples/ackermann.asm", values );
}

void Test::find()
{
  Sequence expected( { 0, 1, 1, 2, 3, 5, 8, 13 } );
//  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
//    17711, 28657, 46368, 75025};

  Finder finder;
  FixedSequenceScorer scorer(expected);
  finder.find( scorer, expected.size(), 23 );

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
  auto fib = parser.parse( "examples/fibonacci.asm" );
  printer.print( fib, std::cout );
}

void Test::all()
{
//  Iterate();
//  Find();
  fibonacci();
  exponentiation();
  ackermann();
}

void Test::testBinary( const std::string& func, const std::string& file,
    const std::vector<std::vector<number_t> >& values )
{
  std::cout << "Running tests for " << file << "..." << std::endl;
  Parser parser;
  Interpreter interpreter;
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
