#pragma once

#include <memory>
#include <string>
#include <vector>

#include "value.hpp"

class Operand
{
public:
  enum class Type
  {
    CONSTANT = 0,
    MEM_ACCESS_DIRECT,
    MEM_ACCESS_INDIRECT
  };

  Operand( Type t, Value v )
      : type( t ),
        value( v )
  {
  }

  Type type;
  Value value;

};

class Operation
{
public:
  enum class Type
  {
    NOP = 0,
    DBG,
    MOV,
    ADD,
    SUB,
    LPB,
    LPE
  };

  Operation()
      : Operation( Type::NOP )
  {
  }

  Operation( Type y )
      : Operation( y, { Operand::Type::CONSTANT, 0 }, { Operand::Type::CONSTANT, 0 } )
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
  using UPtr = std::unique_ptr<Program>;

  std::vector<Operation> ops;
};
