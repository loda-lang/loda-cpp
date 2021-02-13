#pragma once

#include "finder.hpp"
#include "interpreter.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "oeis_sequence.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class Oeis
{
public:

  Oeis( const Settings &settings );

  virtual ~Oeis()
  {
  }

  void load( volatile sig_atomic_t &exit_flag );

  void migrate( volatile sig_atomic_t &exit_flag );

  const std::vector<OeisSequence>& getSequences() const;

  void removeSequence( size_t id );

  Finder& getFinder()
  {
    return finder;
  }

  void dumpProgram( size_t id, Program p, const std::string &file ) const;

  size_t getTotalCount() const
  {
    return total_count_;
  }

  std::pair<bool, bool> updateProgram( size_t id, const Program &p );

  void maintain( volatile sig_atomic_t &exit_flag );

private:

  size_t loadData( volatile sig_atomic_t &exit_flag );

  void loadNames( volatile sig_atomic_t &exit_flag );

  void update( volatile sig_atomic_t &exit_flag );

  std::pair<bool, Program> minimizeAndCheck( const Program &p, const OeisSequence &seq, bool optimize );

  int getNumCycles( const Program &p );

  std::string isOptimizedBetter( Program existing, Program optimized, size_t id );

  const Settings &settings;
  Interpreter interpreter;
  Finder finder;
  Minimizer minimizer;
  Optimizer optimizer;
  std::vector<OeisSequence> sequences;
  size_t total_count_;

};
