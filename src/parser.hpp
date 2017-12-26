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

  std::string ReadVariable();

  Operation::Type ReadOperationType();

  std::string working_dir;
  std::istream* in;
  std::unique_ptr<std::ifstream> file_in;
  Program::UPtr p;

};
