#pragma once

#include "program.hpp"

class Parser
{
public:

  Program::UPtr Parse( const std::string& filename );

  Program::UPtr Parse( std::istream& in );

  void SetWorkingDir( const std::string& dir );

private:

  std::string working_dir;

};
