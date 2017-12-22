//============================================================================
// Name        : cm.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>

#include "program.hpp"

int main( void )
{

  Assignment::Array<2> a;
  a[1] = Assignment( 62, true );

  Program<3> p;

  puts( "Hello World!!!" );
  return EXIT_SUCCESS;
}
