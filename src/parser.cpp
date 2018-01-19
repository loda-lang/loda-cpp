#include "parser.hpp"

#include "program.hpp"
#include "value.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

Program::UPtr Parser::Parse( const std::string& file )
{
  file_in.reset( new std::ifstream( file ) );
  if ( !file_in )
  {
    throw std::runtime_error( "error opening file" );
  }
  auto p = Parse( *file_in );
  file_in->close();
  return p;
}

Program::UPtr Parser::Parse( std::istream& in_ )
{
  in = &in_;
  Program::UPtr p( new Program() );
  Operation::UPtr o;
  std::string l;
  while ( true )
  {
    *in >> std::ws;
    auto c = in->peek();
    if ( c == EOF )
    {
      break;
    }
    o = nullptr;
    if ( c == ';' )
    {
      o.reset( new Nop() );
    }
    else
    {
      // read normal operation
      auto op_type = ReadOperationType();
      *in >> std::ws;
      switch ( op_type )
      {

      case Operation::Type::NOP:
      {
        o.reset( new Nop() );
        break;
      }

      case Operation::Type::DBG:
      {
        o.reset( new Dbg() );
        break;
      }

      case Operation::Type::MOV:
      {
        auto t = ReadOperand( *p );
        ReadSeparator( ',' );
        auto s = ReadOperand( *p );
        o.reset( new Mov( t, s ) );
        break;
      }

      case Operation::Type::ADD:
      {
        auto t = ReadOperand( *p );
        ReadSeparator( ',' );
        auto s = ReadOperand( *p );
        o.reset( new Add( t, s ) );
        break;
      }

      case Operation::Type::SUB:
      {
        auto t = ReadOperand( *p );
        ReadSeparator( ',' );
        auto s = ReadOperand( *p );
        o.reset( new Sub( t, s ) );
        break;
      }

      case Operation::Type::LPB:
      {
        auto t = ReadOperand( *p );
        ReadSeparator( ',' );
        auto s = ReadOperand( *p );
        o.reset( new LoopBegin( t, s ) );
        break;
      }

      case Operation::Type::LPE:
      {
        o.reset( new LoopEnd() );
        break;
      }

      }

    }

    // read comment
    c = in->peek();
    while ( c == ' ' || c == '\t' )
    {
      in->get();
      c = in->peek();
    }
    if ( c == ';' )
    {
      in->get();
      c = in->peek();
      while ( c == ' ' || c == '\t' )
      {
        in->get();
        c = in->peek();
      }
      std::getline( *in, l );
      o->comment = l;
    }

    // add operation to program
    if ( o )
    {
      p->ops.emplace_back( std::move( o ) );
    }
  }
  return p;
}

void Parser::ReadSeparator( char separator )
{
  *in >> std::ws;
  if ( in->get() != separator )
  {
    throw std::runtime_error( "expected separator" );
  }
}

Value Parser::ReadValue()
{
  Value v;
  int c;
  *in >> std::ws;
  c = in->peek();
  if ( !std::isdigit( c ) )
  {
    throw std::runtime_error( "invalid value" );
  }
  *in >> v;
  return v;
}

std::string Parser::ReadIdentifier()
{
  std::string s;
  int c;
  *in >> std::ws;
  c = in->get();
  if ( c == '_' || std::isalpha( c ) )
  {
    s += (char) c;
    while ( true )
    {
      c = in->peek();
      if ( c == '_' || std::isalnum( c ) )
      {
        s += (char) c;
        in->get();
      }
      else
      {
        break;
      }
    }
    std::transform( s.begin(), s.end(), s.begin(), ::tolower );
    return s;
  }
  else
  {
    throw std::runtime_error( "invalid identifier" );
  }
}

Operand Parser::ReadOperand( Program& p )
{
  *in >> std::ws;
  int c = in->peek();
  if ( c == '$' )
  {
    in->get();
    c = in->peek();
    if ( c == '$' )
    {
      in->get();
      return Operand( Operand::Type::MEM_ACCESS_INDIRECT, ReadValue() );
    }
    else
    {
      return Operand( Operand::Type::MEM_ACCESS_DIRECT, ReadValue() );
    }

  }
  else
  {
    return Operand( Operand::Type::CONSTANT, ReadValue() );
  }
}

Operation::Type Parser::ReadOperationType()
{
  auto t = ReadIdentifier();
  if ( t == "nop" )
  {
    return Operation::Type::NOP;
  }
  else if ( t == "dbg" )
  {
    return Operation::Type::DBG;
  }
  else if ( t == "mov" )
  {
    return Operation::Type::MOV;
  }
  else if ( t == "add" )
  {
    return Operation::Type::ADD;
  }
  else if ( t == "sub" )
  {
    return Operation::Type::SUB;
  }
  else if ( t == "lpb" )
  {
    return Operation::Type::LPB;
  }
  else if ( t == "lpe" )
  {
    return Operation::Type::LPE;
  }
  throw std::runtime_error( "invalid operation: " + t );
}

void Parser::SetWorkingDir( const std::string& dir )
{
  working_dir = dir;
}
