#pragma once

#include <array>
#include <stack>

#include "program.hpp"
#include "memory.hpp"

class Interpreter
{
public:

  using MemStack = std::stack<Memory>;
  using PCStack = std::stack<size_t>;

  bool Run( Program& p, Memory& mem )
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
      size_t ps_next = pc + 1;
      switch ( op->GetType() )
      {
      case Operation::Type::CMT:
      {
        break;
      }
      case Operation::Type::SET:
      {
        auto set = Set::Cast( op );
        mem.regs[set->target_var] = set->value;
        break;
      }
      case Operation::Type::CPY:
      {
        auto copy = Copy::Cast( op );
        mem.regs[copy->target_var] = mem.regs[copy->source_var];
        break;
      }
      case Operation::Type::ADD:
      {
        auto add = Add::Cast( op );
        mem.regs[add->target_var] += mem.regs[add->source_var];
        break;
      }
      case Operation::Type::SUB:
      {
        auto sub = Sub::Cast( op );
        mem.regs[sub->target_var] =
            (mem.regs[sub->target_var] > mem.regs[sub->source_var]) ?
                (mem.regs[sub->target_var] - mem.regs[sub->source_var]) : 0;
        break;
      }
      case Operation::Type::LPB:
      {
        loop_stack.push( pc );
        mem_stack.push( mem );
        break;
      }
      case Operation::Type::LPE:
      {
        auto ps_begin = loop_stack.top();
        loop_stack.pop();
        auto loop_begin = LoopBegin::Cast( p.ops[ps_begin] );
        auto prev = mem_stack.top();
        mem_stack.pop();
        if ( mem.IsLessThan( prev, loop_begin->loop_vars ) )
        {
          ps_next = ps_begin;
        }
        else
        {
          mem = prev;
        }
        break;
      }
      }
      if ( ps_next < p.ops.size() )
      {
        pc_stack.push( ps_next );
      }
    }
    return true;
  }

};
