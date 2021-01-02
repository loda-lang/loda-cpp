#pragma once

#include "number.hpp"

#include <array>
#include <vector>

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

  inline bool operator!=( const Operand &o ) const
  {
    return !((*this) == o);
  }

  inline bool operator<( const Operand &o ) const
  {
    if ( type != o.type ) return type < o.type;
    if ( value != o.value ) return value < o.value;
    return false; // equal
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
    TRN,
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

  static const std::array<Type, 18> Types;

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

  inline bool operator!=( const Operation &op ) const
  {
    return !((*this) == op);
  }

  inline bool operator<( const Operation &op ) const
  {
    if ( type != op.type ) return type < op.type;
    if ( target != op.target ) return target < op.target;
    if ( source != op.source ) return source < op.source;
    return false; // equal
  }

  Type type;
  Operand target;
  Operand source;
  std::string comment;
};

class Program
{
public:

  static constexpr int64_t INPUT_CELL = 0;
  static constexpr int64_t OUTPUT_CELL = 1;

  void push_front( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv );

  void push_back( Operation::Type t, Operand::Type tt, number_t tv, Operand::Type st, number_t sv );

  bool operator==( const Program &p ) const;

  bool operator!=( const Program &p ) const;

  std::vector<Operation> ops;
};
