#pragma once

#include "program.hpp"

#include <iostream>

class Serializer
{

  void writeProgram( const Program& p, std::ostream& out );

  void readProgram( Program& p, std::istream& in );

};
