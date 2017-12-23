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
  virtual Type getType() const = 0;
};

class UnaryOperation: public Operation
{
public:
  const_t value;
  var_t target_var;
};

class BinaryOperation: public Operation
{
public:
  var_t source_var;
  var_t target_var;
};

class Set: public UnaryOperation
{
public:
  virtual Type getType() const override
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
  virtual Type getType() const override
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
  virtual Type getType() const override
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
  virtual Type getType() const override
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
  virtual Type getType() const override
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
  virtual Type getType() const override
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
