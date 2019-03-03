#pragma once

#include "number.hpp"
#include "util.hpp"
#include "program.hpp"
#include "oeis.hpp"

#include <csignal>

class Miner
{
public:

  Miner( const Settings& settings );

  void mine( volatile sig_atomic_t& exit_flag );

private:

  const Settings& settings;

  Oeis oeis;

};
