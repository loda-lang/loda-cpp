#pragma once

#include "finder.hpp"
#include "interpreter.hpp"
#include "minimizer.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class OeisSequence: public Sequence
{
public:

  OeisSequence( size_t id = 0 )
      :
      id( id )
  {
  }

  OeisSequence( size_t id, const std::string &name, const Sequence &s, const Sequence &full )
      :
      Sequence( s ),
      id( id ),
      name( name ),
      full( full )
  {
  }

  std::string id_str( const std::string &prefix = "A" ) const;

  std::string dir_str() const;

  std::string getProgramPath() const;

  std::string getBFilePath() const;

  size_t id;
  std::string name;
  Sequence full;

  friend std::ostream& operator<<( std::ostream &out, const OeisSequence &s );

  std::string to_string() const;

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

  std::pair<bool, bool> updateProgram( size_t id, const Program &p ) const;

  void maintain( volatile sig_atomic_t &exit_flag );

private:

  void loadNames( volatile sig_atomic_t &exit_flag );

  std::pair<bool, Program> minimizeAndCheck( const Program &p, const OeisSequence &seq, bool optimize ) const;

  int getNumCycles( const Program &p ) const;

  std::string isOptimizedBetter( Program existing, Program optimized ) const;

  const Settings &settings;
  Interpreter interpreter;
  Finder finder;
  Minimizer minimizer;
  Optimizer optimizer;
  std::vector<OeisSequence> sequences;
  size_t total_count_;

};