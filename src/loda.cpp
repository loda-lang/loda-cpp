#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "test.hpp"

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
}

int main( int argc, char *argv[] )
{
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

