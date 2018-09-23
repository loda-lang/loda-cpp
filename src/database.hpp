#pragma once

#include "common.hpp"
#include "program.hpp"

class Database
{
public:

  bool insert( Program&& p );

  void save();

  void printPrograms();

  void printSequences();

private:

  std::vector<std::pair<Program, Sequence>> programs_;

};
