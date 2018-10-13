#include "interpreter.hpp"

#include "printer.hpp"

#include <array>
#include <iostream>
#include <sstream>
#include <stack>
#include "number.hpp"

using MemStack = std::stack<Memory>;
using SizeStack = std::stack<size_t>;

Interpreter::Interpreter( const Settings& settings )
    : settings( settings ),
      is_debug( Log::get().level == Log::Level::DEBUG )
{
}

bool Interpreter::run( const Program& p, Memory& mem ) const
{
  // check for empty program
  if ( p.ops.empty() )
  {
    return true;
  }

  // define stacks
  SizeStack pc_stack;
  SizeStack loop_stack;
  SizeStack frag_length_stack;
  MemStack mem_stack;
  MemStack frag_stack;

  // push first operation to stack
  pc_stack.push( 0 );

  Printer printer;

  number_t cycles = 0;
  Memory old_mem, frag, frag_prev, prev;
  size_t pc, pc_next, ps_begin;
  number_t source, target, start, length, length2;
  Operation lpb;

  // loop until stack is empty
  while ( !pc_stack.empty() )
  {
    if ( is_debug )
    {
      old_mem = mem;
    }

    pc = pc_stack.top();
    pc_stack.pop();
    auto& op = p.ops.at( pc );
    pc_next = pc + 1;

    switch ( op.type )
    {
    case Operation::Type::NOP:
    {
      break;
    }
    case Operation::Type::MOV:
    {
      source = get( op.source, mem );
      set( op.target, source, mem );
      break;
    }
    case Operation::Type::ADD:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, target + source, mem );
      break;
    }
    case Operation::Type::SUB:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, (target > source) ? (target - source) : 0, mem );
      break;
    }
    case Operation::Type::LPB:
    {
      length = get( op.source, mem );
      start = get( op.target, mem, true );
      if ( length > settings.max_memory )
      {
        Log::get().error( "Maximum memory fragment length exceeded: " + std::to_string( length ), true );
      }
      frag = mem.fragment( start, length );
      loop_stack.push( pc );
      mem_stack.push( mem );
      frag_stack.push( frag );
      frag_length_stack.push( length );
      break;
    }
    case Operation::Type::LPE:
    {
      ps_begin = loop_stack.top();
      lpb = p.ops[ps_begin];
      prev = mem_stack.top();
      mem_stack.pop();

      frag_prev = frag_stack.top();
      frag_stack.pop();

      length = frag_length_stack.top();
      frag_length_stack.pop();

      start = get( lpb.target, mem, true );
      length2 = get( lpb.source, mem );
      if ( length > length2 )
      {
        length = length2;
      }

      frag = mem.fragment( start, length );

      if ( frag < frag_prev )
      {
        pc_next = ps_begin + 1;
        mem_stack.push( mem );
        frag_stack.push( frag );
        frag_length_stack.push( length );
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

    if ( is_debug )
    {
      std::stringstream buf;
      buf << "Executing ";
      printer.print( op, buf );
      buf << " " << old_mem << " => " << mem;
      Log::get().debug( buf.str() );
    }

    if ( ++cycles >= settings.max_cycles )
    {
      Log::get().error( "Program did not terminated after " + std::to_string( cycles ) + " cycles", true );
    }
  }
  return true;
}

number_t Interpreter::get( Operand a, const Memory& mem, bool getAddress ) const
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

void Interpreter::set( Operand a, number_t v, Memory& mem ) const
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
    Log::get().error( "Memory index out of range: " + std::to_string( index ), true );
  }
  mem.set( index, v );
}

bool Interpreter::isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmpVars ) const
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

Sequence Interpreter::eval( const Program& p, int num_terms ) const
{
  if ( num_terms < 0 )
  {
    num_terms = settings.num_terms;
  }
  Sequence seq;
  seq.resize( num_terms );
  Memory mem;
  for ( number_t i = 0; i < num_terms; i++ )
  {
    mem.clear();
    mem.set( 0, i );
    run( p, mem );
    seq[i] = mem.get( 1 );
  }
  if ( is_debug )
  {
    std::stringstream buf;
    buf << "Evaluated program to sequence " << seq;
    Log::get().debug( buf.str() );
  }
  return seq;
}
