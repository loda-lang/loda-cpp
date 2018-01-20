#include "test.hpp"

#include "evaluator.hpp"
#include "interpreter.hpp"
#include "parser.hpp"

void Test::Fibonacci()
{
  std::cout << "Running Fibonacci test..." << std::endl;

  Sequence expected;
  expected.data =
  { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946,
    17711, 28657, 46368, 75025};

  Parser parser;
  Evaluator evaluator;

  auto fib = parser.Parse( "examples/fibonacci.loda" );
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
  std::cout << "Running Exponentiation test..." << std::endl;

}

void Test::Ackermann()
{
  std::cout << "Running Ackermann test..." << std::endl;

  std::vector<std::vector<Value> > values;
  values =
  {
    { 1,2,3,4,5},
    { 2,3,4,5,6},
    { 3,5,7,9,11},
    { 5,13,29,61,125},
    { 13,65533}
  };

  Parser parser;
  Interpreter interpreter;

  auto ack = parser.Parse( "examples/ackermann.loda" );

  for ( uint64_t i = 0; i < values.size(); i++ )
  {
    for ( uint64_t n = 0; n < values[i].size(); n++ )
    {
      std::cout << "ack(" << i << "," << n << ")=";
      Sequence mem;
      mem.Set( 0, i );
      mem.Set( 1, n );
      interpreter.Run( *ack, mem );
      std::cout << mem.Get( 2 ) << std::endl;
      if ( mem.Get( 2 ) != values[i][n] )
      {
        throw std::runtime_error( "unexpected result: " + std::to_string( mem.Get( 2 ) ) );
      }
    }
  }
  std::cout << std::endl;

}

void Test::All()
{
  Fibonacci();
  Exponentiation();
  Ackermann();
}
