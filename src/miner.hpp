#pragma once

#include "number.hpp"
#include "util.hpp"
#include "program.hpp"
#include "oeis.hpp"

#include <csignal>
#include <unordered_map>

class Miner
{
public:

  Miner( const Settings& settings )
      : settings( settings ),
        oeis( settings )
  {
    oeis.load();
  }

  void Mine( volatile sig_atomic_t& exit_flag );

private:
  const Settings& settings;
  Oeis oeis;
  std::unordered_map<number_t, std::pair<double, double>> bounds;

  void CheckBounds( const Program& p );

  double GetDiff( const Sequence& s1, const Sequence& s2 );

  double GetBounds( number_t id, bool upper, const OeisSequence& target_seq, bool force_update );

  void UpdateBoundProgram( number_t id, bool upper, Program p, double bound );

};
