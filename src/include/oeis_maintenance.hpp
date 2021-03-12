#pragma once

#include "oeis_manager.hpp"

class OeisMaintenance
{
public:

  OeisMaintenance( const Settings &settings );

  void maintain();

private:

  void generateStats( const steps_t& steps );

  size_t checkAndMinimizePrograms();

  const Settings &settings;
  Interpreter interpreter;
  OeisManager manager;

};
