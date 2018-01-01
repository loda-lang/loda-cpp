#include "interpreter.hpp"

#include <iostream>
#include <array>
#include <stack>

using MemStack = std::stack<Memory>;
using PCStack = std::stack<size_t>;

bool Interpreter::Run( Program& p, Memory& mem )
{
  // check for empty program
  if ( p.ops.empty() )
  {
    return true;
  }

  // define stacks
  PCStack pc_stack;
  PCStack loop_stack;
  MemStack mem_stack;
  MemStack frag_stack;

  // push first operation to stack
  pc_stack.push( 0 );

  // loop until stack is empty
  while ( !pc_stack.empty() )
  {
    std::cout << mem << std::endl;

    size_t pc = pc_stack.top();
    pc_stack.pop();
    auto& op = p.ops.at( pc );
    size_t pc_next = pc + 1;
    switch ( op->GetType() )
    {
    case Operation::Type::NOP:
    {
      break;
    }
    case Operation::Type::MOV:
    {
//      std::cout << "MOV" << std::endl;
      auto mov = Mov::Cast( op );
      auto s = Get( mov->source, mem );
      Set( mov->target, s, mem );
      break;
    }
    case Operation::Type::ADD:
    {
//      std::cout << "ADD" << std::endl;
      auto add = Add::Cast( op );
      auto s = Get( add->source, mem );
      auto t = Get( add->target, mem );
      Set( add->target, t + s, mem );
      break;
    }
    case Operation::Type::SUB:
    {
//      std::cout << "SUB" << std::endl;
      auto sub = Sub::Cast( op );
      auto s = Get( sub->source, mem );
      auto t = Get( sub->target, mem );
      Set( sub->target, (t > s) ? (t - s) : 0, mem );
      break;
    }
    case Operation::Type::LPB:
    {
      std::cout << "LPB" << std::endl;
      auto lpb = LoopBegin::Cast( op );
      auto l = Get( lpb->source, mem );
      auto s = Get( lpb->target, mem );
      loop_stack.push( pc );
      mem_stack.push( mem );
      frag_stack.push( mem.Fragment( s, l ) );
      break;
    }
    case Operation::Type::LPE:
    {
      std::cout << "LPE" << std::endl;

      auto ps_begin = loop_stack.top();
      auto lpb = LoopBegin::Cast( p.ops[ps_begin] );
      auto prev = mem_stack.top();
      mem_stack.pop();

      auto frag_prev = frag_stack.top();
      frag_stack.pop();

      auto l = Get( lpb->source, mem );
      auto s = Get( lpb->target, mem );
      auto frag = mem.Fragment( s, l );

      if ( frag < frag_prev )
      {
        pc_next = ps_begin + 1;
        mem_stack.push( mem );
        frag_stack.push( frag );
      }
      else
      {
        mem = prev;
        loop_stack.pop();
      }
      break;
    }
    }
    if ( pc_next < p.ops.size() )
    {
      pc_stack.push( pc_next );
    }
  }
  return true;
}

Value Interpreter::Get( Operand a, const Memory& mem )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
    return a.value;
  case Operand::Type::MEM_ACCESS_DIRECT:
    return mem.Get( a.value );
  case Operand::Type::MEM_ACCESS_INDIRECT:
    return mem.Get( mem.Get( a.value ) );
  }
  return
  {};
}

void Interpreter::Set( Operand a, Value v, Memory& mem )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
    throw std::runtime_error( "cannot set value to constant" );
  case Operand::Type::MEM_ACCESS_DIRECT:
    mem.Set( a.value, v );
    break;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    mem.Set( mem.Get( a.value ), v );
    break;
  }
}

bool Interpreter::IsLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmp_vars )
{
  for ( Operand v : cmp_vars )
  {
    if ( Get( v, m1 ) < Get( v, m2 ) )
    {
      return true; // less
    }
    else if ( Get( v, m1 ) > Get( v, m2 ) )
    {
      return false; // greater
    }
  }
  return false; // equal
}
