#pragma once

#include <array>
#include <stack>

#include "program.hpp"

class Interpreter
{
public:

  using State = std::array<value_t,256>;
  using PCStack = std::stack<size_t>;

  bool Run( Program& p, State& s )
  {
    if ( p.ops.empty() )
    {
      return true;
    }
    PCStack pcs;
    pcs.push( 0 );
    while ( !pcs.empty() )
    {
      size_t pc = pcs.top();
      pcs.pop();
      auto& op = p.ops.at( pc );
      bool push_next;
      switch ( op->GetType() )
      {
      case Operation::Type::SET:
      {
        auto set = Set::Cast( op );
        s[set->target_var] = set->value;
        push_next = true;
        break;
      }
      case Operation::Type::CPY:
      {
        auto copy = Copy::Cast( op );
        s[copy->target_var] = s[copy->source_var];
        push_next = true;
        break;
      }
      case Operation::Type::ADD:
      {
        auto add = Add::Cast( op );
        s[add->target_var] += s[add->source_var];
        push_next = true;
        break;
      }
      case Operation::Type::SUB:
      {
        auto sub = Sub::Cast( op );
        s[sub->target_var] = (s[sub->target_var] > s[sub->source_var]) ? (s[sub->target_var] - s[sub->source_var]) : 0;
        push_next = true;
        break;
      }
      case Operation::Type::LPS:
      {
        auto loop_start = LoopStart::Cast( op );

        push_next = false;
        break;
      }
      case Operation::Type::LPE:
      {
        auto loop_end = LoopEnd::Cast( op );

        push_next = false;
        break;
      }
      }
      if ( push_next && pc + 1 < p.ops.size() )
      {
        pcs.push( pc + 1 );
      }
    }
    return true;
  }

};
