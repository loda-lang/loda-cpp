#pragma once

#include "number.hpp"
#include "program.hpp"

#include <map>

class ProgramUtil
{
public:

  static void removeOps( Program &p, Operation::Type type );

  static bool replaceOps( Program &p, Operation::Type oldType, Operation::Type newType );

  static size_t numOps( const Program &p, bool withNops );

  static size_t numOps( const Program &p, Operation::Type type );

  static size_t numOps( const Program &p, Operand::Type type );

  static std::string operandToString( const Operand &op );

  static std::string operationToString( const Operation &op );

  static void print( const Program &p, std::ostream &out );

  static void print( const Operation &op, std::ostream &out, int indent = 0 );

  static size_t hash( const Program &p );

  static size_t hash( const Operation &op );

  static size_t hash( const Operand &op );

};
