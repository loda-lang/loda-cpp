#pragma once

#include "interpreter.hpp"
#include "matcher.hpp"
#include "number.hpp"
#include "optimizer.hpp"
#include "program.hpp"
#include "util.hpp"

class OeisSequence: public Sequence
{
public:

  OeisSequence( number_t id = 0 )
      :
      id( id )
  {
  }

  OeisSequence( number_t id, const std::string &name, const Sequence &s, const Sequence &full )
      :
      Sequence( s ),
      id( id ),
      name( name ),
      full( full )
  {
  }

  std::string id_str( const std::string &prefix = "A" ) const;

  number_t id;
  std::string name;
  Sequence full;

  friend std::ostream& operator<<( std::ostream &out, const OeisSequence &s );

  std::string to_string() const;

};

class Oeis
{
public:

  static size_t MAX_NUM_TERMS;

  using seq_programs_t = std::vector<std::pair<number_t,Program>>;

  Oeis( const Settings &settings );

  virtual ~Oeis()
  {
  }

  void load();

  const std::vector<OeisSequence>& getSequences() const;

  void removeSequence( number_t id );

  seq_programs_t findSequence( const Program &p, Sequence &norm_seq ) const;

  void dumpProgram( number_t id, Program p, const std::string &file ) const;

  size_t getTotalCount() const
  {
    return total_count_;
  }

  std::pair<bool, bool> updateProgram( number_t id, const Program &p ) const;

  void maintain( volatile sig_atomic_t &exit_flag );

private:

  void loadNames();

  void findAll( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const;

  std::pair<bool, Program> optimizeAndCheck( const Program &p, const OeisSequence &seq ) const;

  const Settings &settings;
  Interpreter interpreter;
  Optimizer optimizer;
  std::vector<OeisSequence> sequences;
  std::vector<std::unique_ptr<Matcher>> matchers;
  size_t total_count_;

};
