#include "evaluator.hpp"
#include "interpreter.hpp"

Sequence Evaluator::Eval( Program& p, Value length )
{
  Sequence seq;
  Interpreter i;
  for ( Value index = 0; index < length; index++ )
  {
    Sequence mem;
    mem.Set( 0, index );
    i.Run( p, mem );
    seq.Set( index, mem.Get( 1 ) );
  }
  return seq;
}
