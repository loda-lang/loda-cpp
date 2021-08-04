#pragma once

#include "number.hpp"
#include "program.hpp"

#include <map>
#include <unordered_set>

class ProgramUtil
{
public:

  static void removeOps( Program &p, Operation::Type type );

  static bool replaceOps( Program &p, Operation::Type oldType, Operation::Type newType );

  static bool isNop( const Operation& op );

  static size_t numOps( const Program &p, bool withNops );

  static size_t numOps( const Program &p, Operation::Type type );

  static size_t numOps( const Program &p, Operand::Type type );

  static bool isArithmetic( Operation::Type t );

  static bool hasIndirectOperand( const Operation &op );

  static bool areIndependent( const Operation& op1, const Operation& op2 );

  static bool getUsedMemoryCells( const Program &p, std::unordered_set<int64_t> &used_cells, int64_t &larged_used,
      int64_t max_memory );

  static std::string operandToString( const Operand &op );

  static std::string operationToString( const Operation &op );

  static void print( const Program &p, std::ostream &out, std::string newline = std::string( "\n" ) );

  static void print( const Operation &op, std::ostream &out, int indent = 0 );

  static void exportToDot( const Program& p, std::ostream &out );

  static size_t hash( const Program &p );

  static size_t hash( const Operation &op );

  static size_t hash( const Operand &op );

  static void validate( const Program& p );

};
