#pragma once

#include "number.hpp"

class Operand
{
public:
  enum class Type
  {
    CONSTANT,
    DIRECT,
    INDIRECT
  };

  Operand()
      :
      Operand( Type::CONSTANT, 0 )
  {
  }

  Operand( Type t, number_t v )
      :
      type( t ),
      value( v )
  {
  }

  inline bool operator==( const Operand &o ) const
  {
    return (type == o.type) && (value == o.value);
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
    MUL,
    DIV,
    MOD,
    POW,
    LOG,
    FAC,
    GCD,
    BIN,
    CMP,
    LPB,
    LPE,
    CLR,
    DBG,
  };

  static const std::array<Type, 17> Types;

  class Metadata
  {
  public:
    static const Metadata& get( Type t );

    static const Metadata& get( const std::string &name );

    Type type;
    std::string name;
    char short_name;
    size_t num_operands;
    bool is_public;
    bool is_reading_target;
    bool is_writing_target;
    int64_t rate;
  };

  Operation()
      :
      Operation( Type::NOP )
  {
  }

  Operation( Type y )
      :
      Operation( y, { Operand::Type::DIRECT, 0 }, { Operand::Type::CONSTANT, 0 } )
  {
  }

  Operation( Type y, Operand t, Operand s, const std::string &c = "" )
      :
      type( y ),
      target( t ),
      source( s ),
      comment( c )
  {
  }

  inline bool operator==( const Operation &op ) const
  {
    return (type == op.type) && (source == op.source) && (target == op.target);
  }

  Type type;
  Operand target;
  Operand source;
  std::string comment;
};

class Program
{
public:

  void push_front( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv );

  void push_back( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv );

  bool operator==( const Program &p ) const;

  std::vector<Operation> ops;
};
