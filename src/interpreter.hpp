#pragma once

#include <array>
#include <stack>

#include "program.hpp"

class Interpreter
{
public:

  using State = std::array<value_t,256>;
  using PCStack = std::stack<size_t>;

  bool run( Program& p, State& s )
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
      switch ( op->getType() )
      {
      case Operation::Type::SET:
      {
        Set* set = Set::Cast( op );
        s[set->target_var] = set->value;
        push_next = true;
        break;
      }
      case Operation::Type::CPY:
      {
        Copy* copy = Copy::Cast( op );
        s[copy->target_var] = s[copy->source_var];
        push_next = true;
        break;
      }

      }

    }
    return true;
  }

};
