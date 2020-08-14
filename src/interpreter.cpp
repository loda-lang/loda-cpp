#include "interpreter.hpp"

#include "number.hpp"
#include "program_util.hpp"
#include "semantics.hpp"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <unistd.h>

using MemStack = std::stack<Memory>;
using SizeStack = std::stack<size_t>;

int64_t Interpreter::PROCESS_ID = 0;

Interpreter::Interpreter( const Settings &settings )
    :
    settings( settings ),
    is_debug( Log::get().level == Log::Level::DEBUG )
{
  if ( settings.dump_last_program )
  {
    if ( !PROCESS_ID )
    {
      PROCESS_ID = getpid();
      if ( PROCESS_ID <= 0 )
      {
        Log::get().error( "Error determining process ID", true );
      }
      ensureDir( getLodaHome() + "dump/" );
    }
  }
}

size_t Interpreter::run( const Program &p, Memory &mem ) const
{
  // check for empty program
  if ( p.ops.empty() )
  {
    return 0;
  }

  // define stacks
  SizeStack pc_stack;
  SizeStack loop_stack;
  SizeStack frag_length_stack;
  MemStack mem_stack;
  MemStack frag_stack;

  // push first operation to stack
  pc_stack.push( 0 );

  size_t cycles = 0;
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
      set( op.target, Semantics::add( target, source ), mem );
      break;
    }
    case Operation::Type::SUB:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::sub( target, source ), mem );
      break;
    }
    case Operation::Type::MUL:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::mul( target, source ), mem );
      break;
    }
    case Operation::Type::DIV:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::div( target, source ), mem );
      break;
    }
    case Operation::Type::MOD:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::mod( target, source ), mem );
      break;
    }
    case Operation::Type::POW:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::pow( target, source ), mem );
      break;
    }
    case Operation::Type::LOG:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::log( target, source ), mem );
      break;
    }
    case Operation::Type::FAC:
    {
      target = get( op.target, mem );
      set( op.target, Semantics::fac( target ), mem );
      break;
    }
    case Operation::Type::GCD:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::gcd( target, source ), mem );
      break;
    }
    case Operation::Type::BIN:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::bin( target, source ), mem );
      break;
    }
    case Operation::Type::CMP:
    {
      source = get( op.source, mem );
      target = get( op.target, mem );
      set( op.target, Semantics::cmp( target, source ), mem );
      break;
    }
    case Operation::Type::LPB:
    {
      length = get( op.source, mem );
      start = get( op.target, mem, true );
      if ( length == NUM_INF )
      {
        throw std::runtime_error( "Infinite loop" );
      }
      else if ( length > (number_t) settings.max_memory )
      {
        throw std::runtime_error( "Maximum memory fragment length exceeded: " + std::to_string( length ) );
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

      if ( frag.is_less( frag_prev, length ) )
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
      length = get( op.source, mem );
      start = get( op.target, mem, true );
      if ( length == NUM_INF )
      {
        mem.clear();
      }
      else if ( length > 0 )
      {
        mem.clear( start, length );
      }
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
      ProgramUtil::print( op, buf );
      buf << " " << old_mem << " => " << mem;
      Log::get().debug( buf.str() );
    }

    if ( ++cycles >= settings.max_cycles )
    {
      throw std::runtime_error( "Program did not terminate after " + std::to_string( cycles ) + " cycles" );
    }
  }
  if ( is_debug )
  {
    Log::get().debug( "Finished execution after " + std::to_string( cycles ) + " cycles" );
  }
  return cycles;
}

number_t Interpreter::get( Operand a, const Memory &mem, bool get_address ) const
{
  switch ( a.type )
  {
  case Operand::Type::CONSTANT:
  {
    if ( get_address )
    {
      throw std::runtime_error( "Cannot get address of a constant" );
    }
    return a.value;
  }
  case Operand::Type::DIRECT:
  {
    return get_address ? a.value : mem.get( a.value );
  }
  case Operand::Type::INDIRECT:
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
    throw std::runtime_error( "Cannot set value of a constant" );
    index = 0; // we don't get here
    break;
  case Operand::Type::DIRECT:
    index = a.value;
    break;
  case Operand::Type::INDIRECT:
    index = mem.get( a.value );
    break;
  }
  if ( index > (number_t) settings.max_memory )
  {
    throw std::runtime_error( "Memory index out of range: " + std::to_string( index ) );
  }
  if ( settings.throw_on_overflow && v == NUM_INF )
  {
    throw std::runtime_error( "Overflow in cell: " + std::to_string( index ) );
  }
  mem.set( index, v );
}

void Interpreter::dump( const Program &p ) const
{
  if ( settings.dump_last_program )
  {
    std::ofstream dump( getLodaHome() + "dump/" + std::to_string( PROCESS_ID ) + ".asm" );
    ProgramUtil::print( p, dump );
    dump.close();
  }
}

size_t Interpreter::eval( const Program &p, Sequence &seq, int num_terms ) const
{
  dump( p );
  if ( num_terms < 0 )
  {
    num_terms = settings.num_terms;
  }
  seq.resize( num_terms );
  Memory mem;
  size_t cycles = 0;
  for ( int i = 0; i < num_terms; i++ )
  {
    mem.clear();
    mem.set( 0, i );
    cycles += run( p, mem );
    seq[i] = mem.get( 1 );
  }
  if ( is_debug )
  {
    std::stringstream buf;
    buf << "Evaluated program to sequence " << seq;
    Log::get().debug( buf.str() );
  }
  return cycles;
}

size_t Interpreter::eval( const Program &p, std::vector<Sequence> &seqs, int num_terms ) const
{
  dump( p );
  if ( num_terms < 0 )
  {
    num_terms = settings.num_terms;
  }
  for ( size_t s = 0; s < seqs.size(); s++ )
  {
    seqs[s].resize( num_terms );
  }
  Memory mem;
  size_t cycles = 0;
  for ( int i = 0; i < num_terms; i++ )
  {
    mem.clear();
    mem.set( 0, i );
    cycles += run( p, mem );
    for ( size_t s = 0; s < seqs.size(); s++ )
    {
      seqs[s][i] = mem.get( s );
    }
  }
  return cycles;
}

bool Interpreter::check( const Program &p, const Sequence &expected_seq ) const
{
  dump( p );
  Memory mem;
  for ( size_t i = 0; i < expected_seq.size(); i++ )
  {
    mem.clear();
    mem.set( 0, i );
    run( p, mem );
    if ( mem.get( 1 ) != expected_seq[i] )
    {
      return false;
    }
  }
  return true;
}
