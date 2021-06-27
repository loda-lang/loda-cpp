#pragma once

#include "memory.hpp"
#include "program.hpp"
#include "sequence.hpp"
#include "util.hpp"

#include <unordered_set>

class steps_t
{
public:
  steps_t();
  size_t min;
  size_t max;
  size_t total;
  size_t runs;
  void add( size_t s );
  void add( const steps_t& s );
};

struct pair_hasher
{
  std::size_t operator()( const std::pair<int64_t, Number> &p ) const
  {
    return (p.first << 32) ^ p.second.hash();
  }
};

enum class status_t
{
  OK,
  WARNING,
  ERROR
};

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  static Number calc( const Operation::Type type, const Number& target, const Number& source );

  size_t run( const Program &p, Memory &mem );

  steps_t eval( const Program &p, Sequence &seq, int64_t num_terms = -1, bool throw_on_error = true );

  steps_t eval( const Program &p, std::vector<Sequence> &seqs, int64_t num_terms = -1 );

  std::pair<status_t, steps_t> check( const Program &p, const Sequence &expected_seq,
      int64_t num_terminating_terms = -1, int64_t id = -1 );

private:

  Number get( const Operand& a, const Memory &mem, bool get_address = false ) const;

  void set( const Operand& a, const Number& v, Memory &mem, const Operation &last_op ) const;

  std::pair<Number, size_t> call( int64_t id, const Number& arg );

  const Program& getProgram( int64_t id );

  const Settings &settings;

  const bool is_debug;
  bool has_memory;
  size_t num_memory_checks;

  std::unordered_map<int64_t, Program> program_cache;
  std::unordered_set<int64_t> missing_programs;
  std::unordered_set<int64_t> running_programs;

  std::unordered_map<std::pair<int64_t, Number>, std::pair<Number, size_t>, pair_hasher> terms_cache;

};
