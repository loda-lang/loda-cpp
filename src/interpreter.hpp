#pragma once

#include <stack>

#include "program.hpp"

class Interpreter
{
public:

  using State = std::array<value_t,256>;
  using Stack = std::stack<const Operation<NUM_VARS>*>;

  bool run( const Operation<NUM_VARS>& op, State& s )
  {
    Stack t;
    t.push( *op );
    while ( !t.empty() )
    {
      auto o = t.pop();

      // apply assignments
      for ( size_t i = 0; i < NUM_VARS; i++ )
      {
        int64_t x =
            o.assignments[i].isReset() ?
                o.assignments[i].getValue() : static_cast<int64_t>( s[i] ) + o.assignments[i].getValue();
        s[i] = (x >= 0) ? x : 0;
      }

      // execute loop
      if ( o.loop_body )
      {
        run( *l.body, s );
      }

      // execute next
      if ( o.loop_body )
      {

      }

    }
    return true;
  }

};
