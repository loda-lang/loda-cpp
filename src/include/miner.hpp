#pragma once

#include "generator.hpp"
#include "interpreter.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class Miner
{
public:

  Miner( const Settings &settings );

  void mine();

  static bool isCollatzValuation( const Sequence &seq );

  bool isPrimeSequence( const Sequence &seq ) const;

private:

  bool updateSpecialSequences( const Program &p, const Sequence &seq ) const;

  const Settings &settings;

  Oeis oeis;

  Interpreter interpreter;

  mutable std::unordered_set<number_t> primes_cache;

};
