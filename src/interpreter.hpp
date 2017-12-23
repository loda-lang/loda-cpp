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

  bool Run( Program& p, Memory& s )
  {
    if ( p.ops.empty() )
    {
      return true;
    }
    PCStack pcs;
    pcs.push( 0 );
    PCStack ls;
    MemStack ss;
    while ( !pcs.empty() )
    {
      std::cout << s << std::endl;
      size_t pc = pcs.top();
      pcs.pop();
      auto& op = p.ops.at( pc );
      size_t next;
      switch ( op->GetType() )
      {
      case Operation::Type::SET:
      {
        auto set = Set::Cast( op );
        s.regs[set->target_var] = set->value;
        next = pc + 1;
        break;
      }
      case Operation::Type::CPY:
      {
        auto copy = Copy::Cast( op );
        s.regs[copy->target_var] = s.regs[copy->source_var];
        next = pc + 1;
        break;
      }
      case Operation::Type::ADD:
      {
        auto add = Add::Cast( op );
        s.regs[add->target_var] += s.regs[add->source_var];
        next = pc + 1;
        break;
      }
      case Operation::Type::SUB:
      {
        auto sub = Sub::Cast( op );
        s.regs[sub->target_var] =
            (s.regs[sub->target_var] > s.regs[sub->source_var]) ?
                (s.regs[sub->target_var] - s.regs[sub->source_var]) : 0;
        next = pc + 1;
        break;
      }
      case Operation::Type::LPS:
      {
        ls.push( pc );
        ss.push( s );
        next = pc + 1;
        break;
      }
      case Operation::Type::LPE:
      {
        auto beg = ls.top();
        ls.pop();
        auto loop_start = LoopStart::Cast( p.ops[beg] );
        auto prev = ss.top();
        ss.pop();
        if ( s.IsLessThan( prev, loop_start->loop_vars ) )
        {
          next = beg;
        }
        else
        {
          s = prev;
          next = pc + 1;
        }
        break;
      }
      }
      if ( next < p.ops.size() )
      {
        pcs.push( next );
      }
    }
    return true;
  }

};
