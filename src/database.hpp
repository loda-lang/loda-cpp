#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Database
{
public:

  Database( const Settings& settings );

  bool insert( Program&& p );

  void save();

  void printPrograms();

  void printSequences();

private:

  std::vector<std::pair<Program, Sequence>> programs;

  const Settings& settings;

  bool dirty;

};
