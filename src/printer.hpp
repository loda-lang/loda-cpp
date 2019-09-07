#pragma once

#include "program.hpp"

#include <iostream>

class Printer
{
public:

  void print( const Program &p, std::ostream &out );

  void print( const Operation &op, std::ostream &out, int indent = 0 );

};
