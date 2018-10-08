#pragma once

#include "number.hpp"

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

  bool operator==( const Operand& o ) const
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
      : Operation( y, { Operand::Type::MEM_ACCESS_DIRECT, 0 }, { Operand::Type::CONSTANT, 0 } )
  {
  }

  Operation( Type y, Operand t, Operand s, const std::string& c = "" )
      : type( y ),
        target( t ),
        source( s ),
        comment( c )
  {
  }

  bool operator==( const Operation& op ) const
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

  size_t num_ops( bool withNops ) const
  {
    if ( withNops )
    {
      return ops.size();
    }
    else
    {
      size_t num_ops = 0;
      for ( auto& op : ops )
      {
        if ( op.type != Operation::Type::NOP )
        {
          num_ops++;
        }
      }
      return num_ops;
    }
  }

  bool operator==( const Program& p ) const
  {
    if ( p.ops.size() != ops.size() )
    {
      return false;
    }
    for ( size_t i = 0; i < ops.size(); ++i )
    {
      if ( !(ops[i] == p.ops[i]) )
      {
        return false;
      }
    }
    return true;
  }

  std::vector<Operation> ops;
};
