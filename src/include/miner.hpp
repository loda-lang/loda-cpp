#pragma once

#include "generator.hpp"
#include "interpreter.hpp"
#include "matcher.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class Miner
{
public:

  class Config
  {
  public:

    std::string name;
    OverwriteMode overwrite_mode;
    std::vector<Generator::Config> generators;
    std::vector<Matcher::Config> matchers;
  };

  Miner( const Settings &settings );

  void mine();

  static bool isCollatzValuation( const Sequence &seq );

private:

  bool updateSpecialSequences( const Program &p, const Sequence &seq ) const;

  const Settings &settings;
  OeisManager manager;
  Interpreter interpreter;

};
