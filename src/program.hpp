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
  using UPtr = std::unique_ptr<Operation>;
  enum class Type
  {
    NOP = 0,
    MOV,
    ADD,
    SUB,
    LPB,
    LPE
  };
  virtual ~Operation()
  {
  }
  virtual Type GetType() const = 0;
  std::string comment;
};

class BinaryOperation: public Operation
{
public:
  BinaryOperation( Operand t, Operand s )
      : target( t ),
        source( s )
  {
  }
  static BinaryOperation* Cast( UPtr& p )
  {
    return dynamic_cast<BinaryOperation*>( p.get() );
  }
  Operand target;
  Operand source;
};

class Nop: public Operation
{
public:
  Nop()
  {
  }
  virtual Type GetType() const override
  {
    return Type::NOP;
  }
  static Nop* Cast( UPtr& p )
  {
    return dynamic_cast<Nop*>( p.get() );
  }
};

class Mov: public BinaryOperation
{
public:
  Mov( Operand t, Operand s )
      : BinaryOperation( t, s )
  {
  }
  virtual Type GetType() const override
  {
    return Type::MOV;
  }
  static Mov* Cast( UPtr& p )
  {
    return dynamic_cast<Mov*>( p.get() );
  }
};

class Add: public BinaryOperation
{
public:
  Add( Operand t, Operand s )
      : BinaryOperation( t, s )
  {
  }
  virtual Type GetType() const override
  {
    return Type::ADD;
  }
  static Add* Cast( UPtr& p )
  {
    return dynamic_cast<Add*>( p.get() );
  }

};

class Sub: public BinaryOperation
{
public:
  Sub( Operand t, Operand s )
      : BinaryOperation( t, s )
  {
  }
  virtual Type GetType() const override
  {
    return Type::SUB;
  }
  static Sub* Cast( UPtr& p )
  {
    return dynamic_cast<Sub*>( p.get() );
  }
};

class LoopBegin: public BinaryOperation
{
public:
  LoopBegin( Operand t, Operand s )
      : BinaryOperation( t, s )
  {
  }
  virtual Type GetType() const override
  {
    return Type::LPB;
  }
  static LoopBegin* Cast( UPtr& p )
  {
    return dynamic_cast<LoopBegin*>( p.get() );
  }
};

class LoopEnd: public Operation
{
public:
  virtual Type GetType() const override
  {
    return Type::LPE;
  }
  static LoopEnd* Cast( UPtr& p )
  {
    return dynamic_cast<LoopEnd*>( p.get() );
  }
};

class Program
{
public:
  using UPtr = std::unique_ptr<Program>;

  std::vector<Operation::UPtr> ops;
};
