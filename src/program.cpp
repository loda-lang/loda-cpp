#include "program.hpp"

void Program::removeOps( Operation::Type type )
{
  auto it = ops.begin();
  while ( it != ops.end() )
  {
    if ( it->type == type )
    {
      it = ops.erase( it );
    }
    else
    {
      it++;
    }
  }
}

size_t Program::num_ops( bool withNops ) const
{
  if ( withNops )
  {
    return ops.size();
  }
  else
  {
    size_t num_ops = 0;
    for ( auto& op : ops )
    {
      if ( op.type != Operation::Type::NOP )
      {
        num_ops++;
      }
    }
    return num_ops;
  }
}

bool Program::operator==( const Program& p ) const
{
  if ( p.ops.size() != ops.size() )
  {
    return false;
  }
  for ( size_t i = 0; i < ops.size(); ++i )
  {
    if ( !(ops[i] == p.ops[i]) )
    {
      return false;
    }
  }
  return true;
}
