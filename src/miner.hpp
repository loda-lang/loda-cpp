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

private:

  Program optimizeAndCheck( const Program& p, const OeisSequence& seq ) const;

  bool updateProgram( number_t id, const Program& p ) const;

  bool updateCollatz( const Program& p, const Sequence& seq ) const;

  bool isCollatzValuation( const Sequence& seq ) const;

  const Settings& settings;

  Oeis oeis;

  Interpreter interpreter;

  Optimizer optimizer;

};
