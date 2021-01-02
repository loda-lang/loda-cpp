#pragma once

#include "program.hpp"

#include <memory>

class Parser
{
public:

  Parser()
      :
      in( nullptr )
  {
  }

  Program parse( const std::string &file );

  Program parse( std::istream &in );

  void readSeparator( char separator );

  number_t readValue();

  std::string readIdentifier();

  Operand readOperand();

  Operation::Type readOperationType();

  std::istream *in;
  std::shared_ptr<std::ifstream> file_in;

};
