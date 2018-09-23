#pragma once

#include "program.hpp"

#include <iostream>

class Serializer
{
public:

  void writeProgram( const Program& p, std::ostream& out );

  void readProgram( Program& p, std::istream& in );

};
