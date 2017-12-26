#include "parser.hpp"

#include <fstream>

Program::UPtr Parser::Parse( const std::string& filename )
{
  std::ifstream in( filename );
  if ( !in )
  {
    throw std::runtime_error( "error opening file" );
  }
  auto p = Parse( in );
  in.close();
  return p;
}

Program::UPtr Parser::Parse( std::istream& in )
{
  Program::UPtr p( new Program() );
  std::string line;
  while ( std::getline( in, line ) )
  {

  }
  return p;
}

void Parser::SetWorkingDir( const std::string& dir )
{
  working_dir = dir;
}
