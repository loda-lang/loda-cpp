#include "interpreter.hpp"

#include "printer.hpp"

#include <array>
#include <iostream>
#include <sstream>
#include <stack>
#include "number.hpp"

using MemStack = std::stack<Memory>;
using PCStack = std::stack<size_t>;

Interpreter::Interpreter( const Settings& settings )
    : settings( settings )
{
}

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

  Printer printer;

  // loop until stack is empty
  number_t cycles = 0;
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

    if ( Log::get().level == Log::Level::DEBUG )
    {
      std::stringstream buf;
      buf << "Executing ";
      printer.print( op, buf );
      buf << " " << old << " => " << mem;
      Log::get().debug( buf.str() );
    }

    if ( ++cycles >= settings.max_cycles )
    {
      Log::get().error( "Program not terminated after " + std::to_string( cycles ) + " cycles", true );
    }
  }
  return true;
}

number_t Interpreter::get( Operand a, const Memory& mem, bool getAddress )
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
  {
    if ( getAddress )
    {
      Log::get().error( "Cannot get address of a constant", true );
    }
    return a.value;
  }
  case Operand::Type::MEM_ACCESS_DIRECT:
  {
    return getAddress ? a.value : mem.get( a.value );
  }
  case Operand::Type::MEM_ACCESS_INDIRECT:
  {
    return getAddress ? mem.get( a.value ) : mem.get( mem.get( a.value ) );
  }
  }
  return
  {};
}

void Interpreter::set( Operand a, number_t v, Memory& mem )
{
  number_t index;
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
    Log::get().error( "Cannot set value of a constant", true );
    index = 0; // we don't get here
    break;
  case Operand::Type::MEM_ACCESS_DIRECT:
    index = a.value;
    break;
  case Operand::Type::MEM_ACCESS_INDIRECT:
    index = mem.get( a.value );
    break;
  }
  if ( index > settings.max_memory )
  {
    Log::get().error( "Memory index " + std::to_string( index ) + " is out of range", true );
  }
  mem.set( index, v );
}

bool Interpreter::isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmpVars )
{
  for ( Operand v : cmpVars )
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

Sequence Interpreter::eval( const Program& p )
{
  Sequence seq;
  seq.resize( settings.num_terms );
  for ( number_t i = 0; i < seq.size(); i++ )
  {
    Memory mem;
    mem.set( 0, i );
    run( p, mem );
    seq[i] = mem.get( 1 );
  }
  if ( Log::get().level == Log::Level::DEBUG )
  {
    std::stringstream buf;
    buf << "Evaluated program to sequence " << seq;
    Log::get().debug( buf.str() );
  }
  return seq;
}
