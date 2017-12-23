#pragma once

#include "types.hpp"

#include <vector>
#include <memory>

class Operation
{
public:
  using UPtr = std::unique_ptr<Operation>;
  enum class Type
  {
    SET,
    CPY,
    ADD,
    SUB,
    LPS,
    LPE
  };
  virtual ~Operation()
  {
  }
  virtual Type GetType() const = 0;
};

class UnaryOperation: public Operation
{
public:
  UnaryOperation( const_t v, var_t t )
      : value( v ),
        target_var( t )
  {
  }
  const_t value;
  var_t target_var;
};

class BinaryOperation: public Operation
{
public:
  BinaryOperation( var_t s, var_t t )
      : source_var( s ),
        target_var( t )
  {
  }
  var_t source_var;
  var_t target_var;
};

class Set: public UnaryOperation
{
public:
  Set( const_t v, var_t t )
      : UnaryOperation( v, t )
  {
  }
  virtual Type GetType() const override
  {
    return Type::SET;
  }
  static Set* Cast( UPtr& p )
  {
    return dynamic_cast<Set*>( p.get() );
  }
};

class Copy: public BinaryOperation
{
public:
  Copy( var_t s, var_t t )
      : BinaryOperation( s, t )
  {
  }
  virtual Type GetType() const override
  {
    return Type::CPY;
  }
  static Copy* Cast( UPtr& p )
  {
    return dynamic_cast<Copy*>( p.get() );
  }
};

class Add: public BinaryOperation
{
public:
  Add( var_t s, var_t t )
      : BinaryOperation( s, t )
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
  Sub( var_t s, var_t t )
      : BinaryOperation( s, t )
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

class LoopStart: public Operation
{
public:
  LoopStart( const std::vector<var_t>& l )
      : loop_vars( l )
  {
  }
  virtual Type GetType() const override
  {
    return Type::LPS;
  }
  static LoopStart* Cast( UPtr& p )
  {
    return dynamic_cast<LoopStart*>( p.get() );
  }
  std::vector<var_t> loop_vars;
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
