#include "parser.hpp"

#include "program.hpp"
#include "util.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include "number.hpp"

#include <stdexcept>

Program Parser::parse( const std::string &file )
{
  file_in.reset( new std::ifstream( file ) );
  if ( !file_in->good() )
  {
    Log::get().error( "Error opening file: " + file, true );
  }
  auto p = parse( *file_in );
  file_in->close();
  return p;
}

Program Parser::parse( std::istream &in_ )
{
  in = &in_;
  Program p;
  Operation o;
  std::string l;
  while ( true )
  {
    *in >> std::ws;
    auto c = in->peek();
    if ( c == EOF )
    {
      break;
    }
    o = Operation();
    if ( c != ';' )
    {
      // read normal operation
      o.type = readOperationType();
      *in >> std::ws;
      if ( o.type == Operation::Type::MOV || o.type == Operation::Type::ADD || o.type == Operation::Type::SUB
          || o.type == Operation::Type::MUL || o.type == Operation::Type::DIV || o.type == Operation::Type::LPB )
      {
        o.target = readOperand( p );
        readSeparator( ',' );
        o.source = readOperand( p );
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
      o.comment = l;
    }

    // add operation to program
    if ( o.type != Operation::Type::NOP || !o.comment.empty() )
    {
      p.ops.push_back( o );
    }
  }
  return p;
}

void Parser::readSeparator( char separator )
{
  *in >> std::ws;
  if ( in->get() != separator )
  {
    throw std::runtime_error( "expected separator" );
  }
}

number_t Parser::readValue()
{
  number_t v;
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

std::string Parser::readIdentifier()
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

Operand Parser::readOperand( Program &p )
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
      return Operand( Operand::Type::MEM_ACCESS_INDIRECT, readValue() );
    }
    else
    {
      return Operand( Operand::Type::MEM_ACCESS_DIRECT, readValue() );
    }

  }
  else
  {
    return Operand( Operand::Type::CONSTANT, readValue() );
  }
}

Operation::Type Parser::readOperationType()
{
  auto name = readIdentifier();
  for ( Operation::Type t : Operation::Types )
  {
    if ( Operation::Metadata::get( t ).name == name )
    {
      return t;
    }
  }
  throw std::runtime_error( "invalid operation: " + name );
}

void Parser::setWorkingDir( const std::string &dir )
{
  working_dir = dir;
}
