#pragma once

#include "number.hpp"
#include "program.hpp"

class ProgramUtil
{
public:

  static void removeOps( Program &p, Operation::Type type );

  static size_t numOps( const Program &p, bool withNops );

  static size_t numOps( const Program &p, Operand::Type type );

  static void print( const Program &p, std::ostream &out );

  static void print( const Operation &op, std::ostream &out, int indent = 0 );

};
