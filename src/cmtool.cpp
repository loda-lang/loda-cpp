#include <iostream>
#include <stdlib.h>

#include "interpreter.hpp"

int main( void )
{
  Program p;
  p.ops.resize( 8 );
  p.ops[0].reset( new Set( 42, 0 ) );
  p.ops[1].reset( new Set( 3, 1 ) );
  p.ops[2].reset( new Set( 1, 2 ) );


  p.ops[3].reset( new LoopBegin( { 1 } ) );
  p.ops[4].reset( new Add( 0, 3 ) );
  p.ops[5].reset( new Sub( 2, 1 ) );
  p.ops[6].reset( new LoopEnd() );
  p.ops[7].reset( new Add( 2, 3 ) );

  Memory m;
  Interpreter i;
  i.Run( p, m );

  std::cout << "out: " << m << std::endl;


  return EXIT_SUCCESS;
}

