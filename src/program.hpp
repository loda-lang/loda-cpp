#pragma once

#include "types.hpp"

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

class Argument
{
public:
  enum class Type
  {
    CONSTANT = 0,
    VARIABLE
  };

  union Value
  {
    var_t variable;
    value_t constant;
  };

  static Argument Constant( value_t value )
  {
    Argument arg;
    arg.type = Type::CONSTANT;
    arg.value.constant = value;
    return arg;
  }

  static Argument Variable( var_t variable )
  {
    Argument arg;
    arg.type = Type::VARIABLE;
    arg.value.variable = variable;
    return arg;
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
  BinaryOperation( var_t t, Argument s )
      : target( t ),
        source( s )
  {
  }
  var_t target;
  Argument source;
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
  Mov( var_t t, Argument s )
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
  Add( var_t t, Argument s )
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
  Sub( var_t t, Argument s )
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

  var_t FindVar( const std::string& var_name ) const
  {
    for ( auto& it : var_names )
    {
      if ( it.second == var_name )
      {
        return it.first;
      }
    }
    throw std::runtime_error( "varible not found: " + var_name );
  }

  std::vector<Operation::UPtr> ops;
  std::unordered_map<var_t, std::string> var_names;
};
