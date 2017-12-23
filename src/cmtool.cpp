#include <iostream>
#include <stdlib.h>

#include "interpreter.hpp"

int main( void )
{
  Program p;
  p.ops.resize( 1 );
  p.ops[0].reset( new Set( 42, 0 ) );

  Memory m;
  Interpreter i;
  i.Run( p, m );

  std::cout << "out: " << m.regs[0];

  return EXIT_SUCCESS;
}

