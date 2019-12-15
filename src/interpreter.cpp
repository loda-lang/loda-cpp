#include "interpreter.hpp"

#include "printer.hpp"

#include <array>
#include <iostream>
#include <sstream>
#include <stack>
#include "number.hpp"

using MemStack = std::stack<Memory>;
using SizeStack = std::stack<size_t>;

inline number_t mul( number_t a, number_t b )
{
  if ( a != NUM_INF && b != NUM_INF && (b == 0 || (NUM_INF / b > a)) )
  {
    return a * b;
  }
  return NUM_INF;
}

Interpreter::Interpreter( const Settings &settings )
    :
    settings( settings ),
    is_debug( Log::get().level == Log::Level::DEBUG )
{
}

bool Interpreter::run( const Program &p, Memory &mem ) const
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
    auto &op = p.ops.at( pc );
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
      if ( NUM_INF - source > target )
      {
        set( op.target, target + source, mem );
      }
      else
      {
        set( op.target, NUM_INF, mem );
      }
      break;
    }
    case Operation::Type::SUB:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      if ( target == NUM_INF || source == NUM_INF )
      {
        set( op.target, NUM_INF, mem );
      }
      else if ( target > source )
      {
        set( op.target, target - source, mem );
      }
      else
      {
        set( op.target, 0, mem );
      }
      break;
    }
    case Operation::Type::MUL:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, mul( target, source ), mem );
      break;
    }
    case Operation::Type::DIV:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      if ( target != NUM_INF && source != NUM_INF && source != 0 )
      {
        set( op.target, target / source, mem );
      }
      else
      {
        set( op.target, NUM_INF, mem );
      }
      break;
    }
    case Operation::Type::MOD:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      if ( target != NUM_INF && source != NUM_INF && source != 0 )
      {
        set( op.target, target % source, mem );
      }
      else
      {
        set( op.target, NUM_INF, mem );
      }
      break;
    }
    case Operation::Type::POW:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Interpreter::pow( target, source ), mem );
      break;
    }
    case Operation::Type::LPB:
    {
      length = get( op.source, mem );
      start = get( op.target, mem, true );
      if ( length == NUM_INF )
      {
        Log::get().error( "Infinite loop", true );
      }
      else if ( length > settings.max_memory )
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
      Log::get().error( "Program did not terminate after " + std::to_string( cycles ) + " cycles", true );
    }
  }
  return true;
}

number_t Interpreter::get( Operand a, const Memory &mem, bool get_address ) const
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
  {
    if ( get_address )
    {
      Log::get().error( "Cannot get address of a constant", true );
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

void Interpreter::set( Operand a, number_t v, Memory &mem ) const
{
  number_t index = 0;
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

bool Interpreter::isLessThan( const Memory &m1, const Memory &m2, const std::vector<Operand> &cmp_vars ) const
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

Sequence Interpreter::eval( const Program &p, int num_terms ) const
{
  if ( num_terms < 0 )
  {
    num_terms = settings.num_terms;
  }
  Sequence seq;
  seq.resize( num_terms );
  Memory mem;
  for ( int i = 0; i < num_terms; i++ )
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

number_t Interpreter::pow( number_t base, number_t exp )
{
  if ( base != NUM_INF && exp != NUM_INF )
  {
    if ( base == 0 )
    {
      return (exp == 0) ? 1 : 0;
    }
    else if ( base == 1 )
    {
      return 1;
    }
    else if ( base > 1 )
    {
      number_t res = 1;
      while ( res != NUM_INF && exp > 0 )
      {
        if ( exp & 1 )
        {
          res = mul( res, base );
        }
        exp >>= 1;
        base = mul( base, base );
      }
      return res;
    }
  }
  return NUM_INF;
}
