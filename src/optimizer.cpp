#include "optimizer.hpp"

#include "number.hpp"

void Optimizer::optimize( Program& p )
{
  removeNops( p );
  while ( removeEmptyLoops( p ) )
    ;
}

bool Optimizer::removeNops( Program& p )
{
  bool removed = false;
  auto it = p.ops.begin();
  while ( it != p.ops.end() )
  {
    if ( it->type == Operation::Type::NOP || it->type == Operation::Type::DBG
        || ((it->type == Operation::Type::ADD || it->type == Operation::Type::SUB)
            && it->source.type == Operand::Type::CONSTANT && it->source.value == 0) )
    {
      it = p.ops.erase( it );
      removed = true;
    }
    else
    {
      ++it;
    }
  }
  return removed;
}

bool Optimizer::removeEmptyLoops( Program& p )
{
  bool removed = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    if ( i + 1 < p.ops.size() && p.ops[i].type == Operation::Type::LPB && p.ops[i + 1].type == Operation::Type::LPE )
    {
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 2 );
      i = i - 2;
      removed = true;
    }
  }
  return removed;
}

