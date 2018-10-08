#include "optimizer.hpp"

#include "interpreter.hpp"
#include "number.hpp"
#include "util.hpp"

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
            && it->source.type == Operand::Type::CONSTANT && it->source.value == 0)
        || (it->type == Operation::Type::MOV && it->source == it->target) )
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
      if ( Log::get().level == Log::Level::DEBUG )
      {
        Log::get().debug( "Removing empty loop" );
      }
      p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 2 );
      i = i - 2;
      removed = true;
    }
  }
  return removed;
}

void Optimizer::minimize( Program& p, size_t num_terms )
{
  Settings settings;
  settings.num_terms = num_terms;
  Interpreter interpreter( settings );
  auto target_sequence = interpreter.eval( p );
  for ( int i = 0; i < (int) p.ops.size(); ++i )
  {
    auto op = p.ops.at( i );
    if ( op.type != Operation::Type::LPB || op.type != Operation::Type::LPE )
    {
      continue;
    }
    p.ops.erase( p.ops.begin() + i, p.ops.begin() + i + 1 );
    auto new_sequence = interpreter.eval( p );
    if ( new_sequence.size() != target_sequence.size() || new_sequence != target_sequence )
    {
      p.ops.insert( p.ops.begin() + i, op );
    }
    else
    {
      --i;
    }
  }
  optimize( p );
}
