#include "interpreter.hpp"

#include "common.hpp"
#include "printer.hpp"

#include <iostream>
#include <array>
#include <stack>

using MemStack = std::stack<Memory>;
using PCStack = std::stack<size_t>;

bool Interpreter::run( const Program& p, Memory& mem )
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
    Memory old = mem;

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
    case Operation::Type::MOV:
    {
      auto s = get( op.source, mem );
      set( op.target, s, mem );
      break;
    }
    case Operation::Type::ADD:
    {
      auto s = get( op.source, mem );
      auto t = get( op.target, mem );
      set( op.target, t + s, mem );
      break;
    }
    case Operation::Type::SUB:
    {
      auto s = get( op.source, mem );
      auto t = get( op.target, mem );
      set( op.target, (t > s) ? (t - s) : 0, mem );
      break;
    }
    case Operation::Type::LPB:
    {
      auto l = get( op.source, mem );
      auto s = get( op.target, mem, true );
      loop_stack.push( pc );
      mem_stack.push( mem );
      frag_stack.push( mem.fragment( s, l ) );
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

      auto l = get( lpb.source, mem );
      auto s = get( lpb.target, mem, true );
      auto frag = mem.fragment( s, l );

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
    case Operation::Type::CLR:
    {
      mem.clear();
      break;
    }
    case Operation::Type::DBG:
    {
      std::cout << mem << std::endl;
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

number_t Interpreter::get( Operand a, const Memory& mem, bool get_address )
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
    return get_address ? a.value : mem.get( a.value );
  }
  case Operand::Type::MEM_ACCESS_INDIRECT:
  {
    return get_address ? mem.get( a.value ) : mem.get( mem.get( a.value ) );
  }
  }
  return
  {};
}

void Interpreter::set( Operand a, number_t v, Memory& mem )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
    throw std::runtime_error( "cannot set value to constant" );
  case Operand::Type::MEM_ACCESS_DIRECT:
    mem.set( a.value, v );
    break;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    mem.set( mem.get( a.value ), v );
    break;
  }
}

bool Interpreter::isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmp_vars )
{
  for ( Operand v : cmp_vars )
  {
    if ( get( v, m1 ) < get( v, m2 ) )
    {
      return true; // less
    }
    else if ( get( v, m1 ) > get( v, m2 ) )
    {
      return false; // greater
    }
  }
  return false; // equal
}

Sequence Interpreter::eval( const Program& p, number_t length )
{
  Sequence seq;
  seq.resize( length );
  for ( number_t index = 0; index < length; index++ )
  {
    Memory mem;
    mem.set( 0, index );
    run( p, mem );
    seq[index] = mem.get( 1 );
  }
  return seq;
}
