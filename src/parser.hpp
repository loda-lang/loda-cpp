#pragma once

#include "program.hpp"

class Parser
{
public:

  Parser()
      : in( nullptr )
  {
  }

  Program Parse( const std::string& file );

  Program Parse( std::istream& in );

  void SetWorkingDir( const std::string& dir );

private:

  void ReadSeparator( char separator );

  number_t ReadValue();

  std::string ReadIdentifier();

  Operand ReadOperand( Program& p );

  Operation::Type ReadOperationType();

  std::string working_dir;
  std::istream* in;
  std::shared_ptr<std::ifstream> file_in;

};
