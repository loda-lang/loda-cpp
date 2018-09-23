#pragma once

#include "common.hpp"
#include "program.hpp"

class Database
{
public:

  void insert( const Program& p );

  void save();

private:

  std::vector<std::pair<Program, Sequence>> programs_;

};
