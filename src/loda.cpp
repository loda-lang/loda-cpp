#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"

int ack( int i, int n )
{
  std::vector<int> next;
  std::vector<int> diff;
  next.resize( i + 1 );
  diff.resize( i + 1 );

  int j;
  for ( j = 0; j <= i; j++ )
  {
    next[j] = 0;
    diff[j] = 1;
  }
  diff[0] = n + 1;

  int value;
  do
  {
    value = next[i] + 1;

    bool transfer = true;

    j = i;
    while ( transfer )
    {
      if ( diff[j] == 0 )
      {
        diff[j] = value - next[j];
      }
      else
      {
        transfer = false;
      }
      next[j]++;
      diff[j]--;
      j--;
    }

    // -----------------------------------
    std::cout << "next:";
    for ( int d = 0; d <= i; d++ )
    {
      std::cout << " " << next[d];
    }
    std::cout << "   diff:";
    for ( int d = 0; d <= i; d++ )
    {
      std::cout << " " << diff[d];
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

