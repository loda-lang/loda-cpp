#pragma once

#include "finder.hpp"
#include "interpreter.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class OeisSequence
{
public:

  OeisSequence( size_t id = 0 )
      : id( id ),
        loaded_bfile( false )
  {
  }

  OeisSequence( size_t id, const std::string &name, const Sequence &s, const Sequence &full )
      : id( id ),
        name( name ),
        norm( s ),
        full( full ),
        loaded_bfile( false )
  {
  }

  std::string id_str( const std::string &prefix = "A" ) const;

  std::string dir_str() const;

  std::string getProgramPath() const;

  std::string getBFilePath() const;

  const Sequence& getFull() const;

  void fetchBFile() const;

  size_t id;
  std::string name;
  Sequence norm;

  friend std::ostream& operator<<( std::ostream &out, const OeisSequence &s );

  std::string to_string() const;

private:

  mutable Sequence full;
  mutable bool loaded_bfile;

};

class Oeis
{
public:

  static size_t MAX_NUM_TERMS;

  Oeis( const Settings &settings );

  virtual ~Oeis()
  {
  }

  void load( volatile sig_atomic_t &exit_flag );

  void update( volatile sig_atomic_t &exit_flag );

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
