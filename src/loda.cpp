#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"

int main( void )
{
  Parser pa;
  auto p = pa.Parse( "test/ackermann.loda" );

  Printer r;

  try
  {

    r.Print( *p, std::cout );
    std::cout << std::endl;

    Memory m;
//    m.regs[p->FindVar("n")] = 8;

    Interpreter i;
    i.Run( *p, m );

    std::cout << std::endl << "out: " << m.regs[p->FindVar("a")] << std::endl;
  }
  catch ( const std::exception& e )
  {
    std::cerr << "error: " << std::string( e.what() ) << std::endl;
    return 1;
  }

  return EXIT_SUCCESS;
}

