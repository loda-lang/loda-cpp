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

  OeisManager( const Settings &settings, bool force_overwrite = false );

  void load();

  void migrate();

  const std::vector<OeisSequence>& getSequences() const;

  const Stats& getStats();

  Finder& getFinder();

  size_t getTotalCount() const
  {
    return total_count;
  }

  std::pair<bool, bool> updateProgram( size_t id, const Program &p );

private:

  void loadData();

  void loadNames();

  static void loadList( const std::string& name, std::unordered_set<size_t>& list );

  bool shouldMatch( const OeisSequence& seq ) const;

  void update();

  void generateStats( int64_t age_in_days );

  void addCalComments( Program& p ) const;

  void dumpProgram( size_t id, Program p, const std::string &file ) const;

  void alert( Program p, size_t id, const std::string& prefix, const std::string& color ) const;

  std::pair<bool, Program> checkAndMinimize( const Program &p, const OeisSequence &seq );

  std::string isOptimizedBetter( Program existing, Program optimized, const OeisSequence &seq );

  friend class OeisMaintenance;

  const Settings &settings;
  const bool overwrite;
  Interpreter interpreter;

  Finder finder;
  bool finder_initialized;

  Minimizer minimizer;
  Optimizer optimizer;
  std::vector<OeisSequence> sequences;
  std::unordered_set<size_t> deny_list;
  std::unordered_set<size_t> protect_list;

  size_t loaded_count;
  size_t ignored_count;
  size_t total_count;

  Stats stats;
  bool stats_loaded;

};
