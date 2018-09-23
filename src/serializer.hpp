#pragma once

#include "program.hpp"

class Serializer
{

  uint16_t writeOperation( const Operation& op );

  Operation readOperation( uint16_t w );

  void writeProgram( const Program& p, std::ostream& out );

  void readProgram( Program& p, std::istream& in );

};
