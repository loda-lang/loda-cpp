#include "interpreter.hpp"

#include <iostream>
#include <array>
#include <stack>

using MemStack = std::stack<Memory>;
using PCStack = std::stack<size_t>;

bool Interpreter::Run( Program& p, Memory& mem )
{
  // check for empty program
  if ( p.ops.empty() )
  {
    return true;
  }

  // define stacks
  PCStack pc_stack;
  PCStack loop_stack;
  MemStack mem_stack;

  // push first operation to stack
  pc_stack.push( 0 );

  // loop until stack is empty
  while ( !pc_stack.empty() )
  {
    std::cout << mem << std::endl;
    size_t pc = pc_stack.top();
    pc_stack.pop();
    auto& op = p.ops.at( pc );
    size_t pc_next = pc + 1;
    switch ( op->GetType() )
    {
    case Operation::Type::NOP:
    {
      break;
    }
    case Operation::Type::MOV:
    {
      std::cout << "mov" << std::endl;
      auto mov = Mov::Cast( op );
      mem.regs[mov->target] = Eval( mov->source, mem );
      break;
    }
    case Operation::Type::ADD:
    {
      std::cout << "add" << std::endl;
      auto add = Add::Cast( op );
      mem.regs[add->target] += Eval( add->source, mem );
      break;
    }
    case Operation::Type::SUB:
    {
      std::cout << "sub" << std::endl;
      auto sub = Sub::Cast( op );
      auto val = Eval( sub->source, mem );
      mem.regs[sub->target] = (mem.regs[sub->target] > val) ? (mem.regs[sub->target] - val) : 0;
      break;
    }
    case Operation::Type::LPB:
    {
      std::cout << "lpb" << std::endl;
      loop_stack.push( pc );
      mem_stack.push( mem );
      break;
    }
    case Operation::Type::LPE:
    {
      std::cout << "lpe" << std::endl;
      auto ps_begin = loop_stack.top();
      loop_stack.pop();
      auto loop_begin = LoopBegin::Cast( p.ops[ps_begin] );
      auto prev = mem_stack.top();
      mem_stack.pop();
      if ( mem.IsLessThan( prev, loop_begin->loop_vars ) )
      {
        pc_next = ps_begin + 1;
      }
      else
      {
        mem = prev;
      }
      break;
    }
    }
    if ( pc_next < p.ops.size() )
    {
      pc_stack.push( pc_next );
    }
  }
  return true;
}

value_t Interpreter::Eval( Argument a, Memory& mem )
{
  switch ( a.type )
  {
  case Argument::Type::CONSTANT:
    return a.value.constant;
  case Argument::Type::VARIABLE:
    return mem.regs[a.value.variable];
  }
}
