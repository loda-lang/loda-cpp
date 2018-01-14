#include "evaluator.hpp"
#include "interpreter.hpp"

Memory Evaluator::Eval( Program& p, Value length )
{
  Memory seq;
  Interpreter i;
  for ( Value index = 0; index < length; index++ )
  {
    Memory mem;
    mem.Set( 0, index );
    i.Run( p, mem );
    seq.Set( index, mem.Get( 1 ) );
  }
  return seq;
}
