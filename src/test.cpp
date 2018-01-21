#include "test.hpp"

#include "evaluator.hpp"
#include "interpreter.hpp"
#include "parser.hpp"

void Test::Fibonacci()
{
  std::cout << "Running tests for examples/fibonacci.asm..." << std::endl;

  Sequence expected;
  expected.data =
  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
    17711, 28657, 46368, 75025};

  Parser parser;
  Evaluator evaluator;

  auto fib = parser.Parse( "examples/fibonacci.asm" );
  auto result = evaluator.Eval( *fib, expected.data.size() );
  std::cout << "fib=" << result << "..." << std::endl;
  if ( result != expected )
  {
    throw std::runtime_error( "unexpected result" );
  }
  std::cout << std::endl;

}

void Test::Exponentiation()
{
  std::vector<std::vector<Value> > values = { { 1, 0, 0 }, { 1, 1, 1 }, { 1, 2, 4 }, { 1, 3, 9 } };
  TestBinary( "exp", "examples/exponentiation.asm", values );

  int m = 3;
  int n = 2;

  int e = 1;
  int f = 0;
  for ( int i = n; i>0; i--)
  {
    for ( int j = m; j>0; j--)
    {
      f = f + e;
    }
    e = f;
    f = 0;
  }
  std::cout << e << std::endl;


}

void Test::Ackermann()
{
  std::vector<std::vector<Value> > values = { { 1, 2, 3, 4, 5 }, { 2, 3, 4, 5, 6 }, { 3, 5, 7, 9, 11 }, { 5, 13, 29, 61,
      125 }, { 13, 65533 } };
  TestBinary( "ack", "examples/ackermann.asm", values );
}

void Test::All()
{
//  Fibonacci();
  Exponentiation();
//  Ackermann();
}

void Test::TestBinary( const std::string& func, const std::string& file,
    const std::vector<std::vector<Value> >& values )
{
  std::cout << "Running tests for " << file << "..." << std::endl;
  Parser parser;
  Interpreter interpreter;
  auto program = parser.Parse( file );
  for ( size_t i = 0; i < values.size(); i++ )
  {
    for ( size_t j = 0; j < values[i].size(); j++ )
    {
      std::cout << func << "(" << i << "," << j << ")=";
      Sequence mem;
      mem.Set( 0, i );
      mem.Set( 1, j );
      interpreter.Run( *program, mem );
      std::cout << mem.Get( 2 ) << std::endl;
      if ( mem.Get( 2 ) != values[i][j] )
      {
        throw std::runtime_error( "unexpected result: " + std::to_string( mem.Get( 2 ) ) );
      }
    }
  }
  std::cout << std::endl;
}
