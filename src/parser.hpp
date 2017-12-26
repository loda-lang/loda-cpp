#pragma once

#include "program.hpp"

class Parser
{
public:

  Parser()
      : in( nullptr )
  {
  }

  Program::UPtr Parse( const std::string& file );

  Program::UPtr Parse( std::istream& in );

  void SetWorkingDir( const std::string& dir );

private:

  void ReadSeparator( char separator );

  value_t ReadInteger();

  std::string ReadIdentifier();

  var_t ReadVariable( Program& p );

  Operation::Type ReadOperationType();

  std::string working_dir;
  std::istream* in;
  std::unique_ptr<std::ifstream> file_in;
  std::unordered_map<std::string, var_t> vars;

};
