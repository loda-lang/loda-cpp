#include "optimizer.hpp"

size_t Optimizer::RemoveEmptyLoops( Program& p )
{
  size_t removed_loops = 0;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    if ( i + 1 < p.ops.size() && p.ops[i]->GetType() == Operation::Type::LPB
        && p.ops[i + 1]->GetType() == Operation::Type::LPE )
    {
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 2 );
      i = i - 2;
      removed_loops++;
    }
  }
  return removed_loops;
}

void Optimizer::Optimize( Program& p )
{
  while ( RemoveEmptyLoops( p ) )
    ;
}
