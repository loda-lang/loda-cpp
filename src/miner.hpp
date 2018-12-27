#pragma once

#include "number.hpp"
#include "util.hpp"

#include <csignal>

class Miner
{
public:

  Miner( const Settings& settings )
      : settings( settings )
  {
  }

  void Mine( volatile sig_atomic_t& exit_flag );

private:
  const Settings& settings;

};
