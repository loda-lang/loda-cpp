#include "interpreter.hpp"

#include "printer.hpp"

#include <iostream>
#include <array>
#include <stack>

using MemStack = std::stack<Sequence>;
using PCStack = std::stack<size_t>;

bool Interpreter::Run( const Program& p, Sequence& mem )
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

  // Printer printer;

  // loop until stack is empty
  while ( !pc_stack.empty() )
  {
    Sequence old = mem;

    size_t pc = pc_stack.top();
    pc_stack.pop();
    auto& op = p.ops.at( pc );
    size_t pc_next = pc + 1;
    switch ( op.type )
    {
    case Operation::Type::NOP:
    {
      break;
    }
    case Operation::Type::DBG:
    {
      std::cout << mem << std::endl;
      break;
    }
    case Operation::Type::MOV:
    {
      auto s = Get( op.source, mem );
      Set( op.target, s, mem );
      break;
    }
    case Operation::Type::ADD:
    {
      auto s = Get( op.source, mem );
      auto t = Get( op.target, mem );
      Set( op.target, t + s, mem );
      break;
    }
    case Operation::Type::SUB:
    {
      auto s = Get( op.source, mem );
      auto t = Get( op.target, mem );
      Set( op.target, (t > s) ? (t - s) : 0, mem );
      break;
    }
    case Operation::Type::LPB:
    {
      auto l = Get( op.source, mem );
      auto s = Get( op.target, mem, true );
      loop_stack.push( pc );
      mem_stack.push( mem );
      frag_stack.push( mem.Fragment( s, l ) );
      break;
    }
    case Operation::Type::LPE:
    {
      auto ps_begin = loop_stack.top();
      auto lpb = p.ops[ps_begin];
      auto prev = mem_stack.top();
      mem_stack.pop();

      auto frag_prev = frag_stack.top();
      frag_stack.pop();

      auto l = Get( lpb.source, mem );
      auto s = Get( lpb.target, mem, true );
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

/*    printer.Print( op, std::cout );
    if ( mem != old )
    {
      std::cout << old << " =>" << std::endl;
      std::cout << mem << std::endl;
    }
    std::cout << std::endl;
*/
  }
  return true;
}

Value Interpreter::Get( Operand a, const Sequence& mem, bool get_address )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
  {
    if ( get_address )
    {
      throw std::runtime_error( "cannot get address of constant" );
    }
    return a.value;
  }
  case Operand::Type::MEM_ACCESS_DIRECT:
  {
    return get_address ? a.value : mem.Get( a.value );
  }
  case Operand::Type::MEM_ACCESS_INDIRECT:
  {
    return get_address ? mem.Get( a.value ) : mem.Get( mem.Get( a.value ) );
  }
  }
  return
  {};
}

void Interpreter::Set( Operand a, Value v, Sequence& mem )
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

bool Interpreter::IsLessThan( const Sequence& m1, const Sequence& m2, const std::vector<Operand>& cmp_vars )
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
