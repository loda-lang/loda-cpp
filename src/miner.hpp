#pragma once

#include "interpreter.hpp"
#include "number.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

#include <csignal>

class Miner
{
public:

  Miner( const Settings& settings );

  void mine( volatile sig_atomic_t& exit_flag );

  void synthesize( volatile sig_atomic_t& exit_flag );

  static bool isCollatzValuation( const Sequence& seq );

private:

  bool updateCollatz( const Program& p, const Sequence& seq ) const;

  const Settings& settings;

  Oeis oeis;

  Interpreter interpreter;

};
