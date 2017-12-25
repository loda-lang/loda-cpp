#pragma once

#include "types.hpp"

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

class Operation
{
public:
  using UPtr = std::unique_ptr<Operation>;
  enum class Type
  {
    CMT = 0,
    SET,
    CPY,
    ADD,
    SUB,
    LPB,
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
  UnaryOperation( var_t t, const_t v )
      : target_var( t ),
        value( v )
  {
  }
  var_t target_var;
  const_t value;
};

class BinaryOperation: public Operation
{
public:
  BinaryOperation( var_t t, var_t s )
      : target_var( t ),
        source_var( s )
  {
  }
  var_t target_var;
  var_t source_var;
};

class Comment: public Operation
{
public:
  Comment( const std::string& v )
      : value( v )
  {
  }
  virtual Type GetType() const override
  {
    return Type::CMT;
  }
  static Comment* Cast( UPtr& p )
  {
    return dynamic_cast<Comment*>( p.get() );
  }
  std::string value;
};

class Set: public UnaryOperation
{
public:
  Set( var_t t, const_t v )
      : UnaryOperation( t, v )
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
  Copy( var_t t, var_t s )
      : BinaryOperation( t, s )
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
  Add( var_t t, var_t s )
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
  Sub( var_t t, var_t s )
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

class LoopBegin: public Operation
{
public:
  LoopBegin( const std::vector<var_t>& l )
      : loop_vars( l )
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
  std::unordered_map<var_t, std::string> var_names;
};
