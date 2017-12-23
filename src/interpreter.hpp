#pragma once

#include <array>
#include <stack>

#include "program.hpp"

class Interpreter
{
public:

  using State = std::array<value_t,256>;
  using StateStack = std::stack<State>;
  using PCStack = std::stack<size_t>;

  bool Run( Program& p, State& s )
  {
    if ( p.ops.empty() )
    {
      return true;
    }
    PCStack pcs;
    pcs.push( 0 );
    PCStack ls;
    StateStack ss;
    while ( !pcs.empty() )
    {
      size_t pc = pcs.top();
      pcs.pop();
      auto& op = p.ops.at( pc );
      size_t next;
      switch ( op->GetType() )
      {
      case Operation::Type::SET:
      {
        auto set = Set::Cast( op );
        s[set->target_var] = set->value;
        next = pc + 1;
        break;
      }
      case Operation::Type::CPY:
      {
        auto copy = Copy::Cast( op );
        s[copy->target_var] = s[copy->source_var];
        next = pc + 1;
        break;
      }
      case Operation::Type::ADD:
      {
        auto add = Add::Cast( op );
        s[add->target_var] += s[add->source_var];
        next = pc + 1;
        break;
      }
      case Operation::Type::SUB:
      {
        auto sub = Sub::Cast( op );
        s[sub->target_var] = (s[sub->target_var] > s[sub->source_var]) ? (s[sub->target_var] - s[sub->source_var]) : 0;
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
        auto loop_end = LoopEnd::Cast( op );
        auto beg = ls.top();
        ls.pop();
        auto loop_start = LoopStart::Cast( p.ops[beg] );
        auto prev = ss.top();
        ss.pop();
        if ( true )
        {
          next = beg;
        }
        else
        {
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
