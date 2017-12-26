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
  Program::UPtr p( new Program() );
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
  *in >> std::ws;
  if ( in->get() != separator )
  {
    throw std::runtime_error( "expected separator" );
  }
}

value_t Parser::ReadInteger()
{
  value_t v;
  int c;
  *in >> std::ws;
  c = in->peek();
  if ( !std::isdigit( c ) )
  {
    throw std::runtime_error( "invalid integer" );
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
    do
    {
      s += (char) c;
      c = in->get();
    } while ( c == '_' || !std::isalnum( c ) );
    std::transform( s.begin(), s.end(), s.begin(), ::tolower );
    return s;
  }
  else
  {
    throw std::runtime_error( "invalid identifier" );
  }
}

var_t Parser::ReadVariable( Program& p )
{
  auto var = ReadIdentifier();
  auto it = vars.find( var );
  if ( it == vars.end() )
  {
    var_t v = vars.size();
    vars[var] = v;
    p.var_names[v] = var;
  }
  return vars[var];
}

Operation::Type Parser::ReadOperationType()
{
  auto t = ReadIdentifier();
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
