#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"

int ack( int i, int n )
{
  std::vector<int> next;
  std::vector<int> goal;
  next.resize( i + 1 );
  goal.resize( i + 1 );
  for ( int d = 0; d <= i; d++ )
  {
    next[d] = 0;
    goal[d] = 1;
  }
  goal[0] = n + 1;
  int value;
  do
  {
    value = next[i] + 1;
    bool transfer = true;
    int j = i;
    while ( transfer )
    {
      if ( next[j] == goal[j] )
      {
        goal[j] = value;
      }
      else
      {
        transfer = false;
      }
      next[j]++;
      j--;
    }

    // -----------------------------------
    std::cout << "next:";
    for ( int d = 0; d <= i; d++ )
    {
      std::cout << " " << next[d];
    }
    std::cout << "   goal:";
    for ( int d = 0; d <= i; d++ )
    {
      std::cout << " " << goal[d];
    }
    std::cout << "\t diff:";
    for ( int d = 0; d <= i; d++ )
    {
      std::cout << " " << (goal[d] - next[d]);
    }
    std::cout << std::endl;
    // -----------------------------------

  } while ( next[0] != n + 1 );
  return value;
}

int main( void )
{

  ack( 3, 3 );

  Parser pa;
  auto p = pa.Parse( "test/fibonacci.loda" );

  Printer r;

  try
  {

    r.Print( *p, std::cout );
    std::cout << std::endl;

    Memory m;
    //    m.regs[p->FindVar("n")] = 8;

    Interpreter i;
    i.Run( *p, m );

    std::cout << std::endl << "out: " << m << std::endl;
  }
  catch ( const std::exception& e )
  {
    std::cerr << "error: " << std::string( e.what() ) << std::endl;
    return 1;
  }

  return EXIT_SUCCESS;
}

