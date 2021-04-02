#pragma once

#include "finder.hpp"
#include "interpreter.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "oeis_sequence.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "stats.hpp"
#include "util.hpp"

class OeisManager
{
public:

  OeisManager( const Settings &settings );

  void load();

  void migrate();

  const std::vector<OeisSequence>& getSequences() const;

  const Stats& getStats();

  Finder& getFinder()
  {
    return finder;
  }

  size_t getTotalCount() const
  {
    return total_count;
  }

  void removeSequenceFromFinder( size_t id );

  std::pair<bool, bool> updateProgram( size_t id, const Program &p );

private:

  size_t loadData();

  void loadNames();

  void loadDenylist();

  void update();

  void generateStats( int64_t age_in_days );

  void addCalComments( Program& p ) const;

  void dumpProgram( size_t id, Program p, const std::string &file ) const;

  std::pair<bool, Program> checkAndMinimize( const Program &p, const OeisSequence &seq, bool minimize );

  std::string isOptimizedBetter( Program existing, Program optimized, const OeisSequence &seq );

  friend class OeisMaintenance;

  const Settings &settings;
  Interpreter interpreter;
  Finder finder;
  Minimizer minimizer;
  Optimizer optimizer;
  std::vector<OeisSequence> sequences;
  std::unordered_set<size_t> denylist;
  size_t total_count;

  Stats stats;
  bool stats_loaded;

};
