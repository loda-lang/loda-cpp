#include <iostream>
#include <stdlib.h>

#include "operation.hpp"

int main( void )
{

  Operation::UPtr o( new Add() );

  /*
   Program<3> p;
   p.operations.resize( 1 );
   p.operations[0].assignments[0] = Assignment( 42, true );

   Interpreter<3>::State s;
   Interpreter<3> i;
   i.run( p, s );

   std::cout << "out: " << s[0];
   */
  return EXIT_SUCCESS;
}

