#include "parser.hpp"

#include <algorithm>
#include <fstream>

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
  p.reset( new Program() );
  Operation::UPtr o;
  int c;
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
    if ( c == '#' )
    {
      // read comment
      in->get();
      *in >> std::ws;
      std::getline( *in, l );
      o.reset( new Comment( l ) );
    }
    else
    {
      // read normal operation
      auto op_type = ReadOperationType();
      *in >> std::ws;
      switch ( op_type )
      {
      case Operation::Type::SET:
      {

      }
      }

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

}

value_t Parser::ReadInteger()
{

}

std::string Parser::ReadIdentifier()
{

}

Operation::Type Parser::ReadOperationType()
{
  auto t = ReadIdentifier();
  std::transform( t.begin(), t.end(), t.begin(), ::tolower );
  if ( t == "set" )
  {
    return Operation::Type::SET;
  }
  else if ( t == "cpy" )
  {
    return Operation::Type::CPY;
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
  throw std::runtime_error( "invalid operation" );
}

void Parser::SetWorkingDir( const std::string& dir )
{
  working_dir = dir;
}
