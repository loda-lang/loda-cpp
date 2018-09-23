#pragma once

#include "common.hpp"

class Operand
{
public:
  enum class Type
  {
    CONSTANT,
    MEM_ACCESS_DIRECT,
    MEM_ACCESS_INDIRECT
  };

  Operand()
      : Operand( Type::CONSTANT, 0 )
  {
  }

  Operand( Type t, number_t v )
      : type( t ),
        value( v )
  {
  }

  Type type;
  number_t value;

};

class Operation
{
public:
  enum class Type
  {
    NOP,
    MOV,
    ADD,
    SUB,
    LPB,
    LPE,
    CLR,
    DBG,
  };

  Operation()
      : Operation( Type::NOP )
  {
  }

  Operation( Type y )
      : Operation( y, { }, { } )
  {
  }

  Operation( Type y, Operand t, Operand s, const std::string& c = "" )
      : type( y ),
        target( t ),
        source( s ),
        comment( c )
  {
  }
  Type type;
  Operand target;
  Operand source;
  std::string comment;
};

class Program
{
public:

  std::vector<Operation> ops;
};
