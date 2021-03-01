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

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  static number_t calc( const Operation::Type type, number_t target, number_t source );

  size_t run( const Program &p, Memory &mem );

  steps_t eval( const Program &p, Sequence &seq, int64_t num_terms = -1 );

  steps_t eval( const Program &p, std::vector<Sequence> &seqs, int64_t num_terms = -1 );

  std::pair<bool, steps_t> check( const Program &p, const Sequence &expected_seq );

private:

  number_t get( Operand a, const Memory &mem, bool get_address = false ) const;

  void set( Operand a, number_t v, Memory &mem, const Operation &last_op ) const;

  Program getProgram( number_t id ) const;

  const Settings &settings;

  const bool is_debug;

  mutable std::unordered_map<number_t, Program> program_cache;
  mutable std::unordered_set<number_t> missing_programs;
  mutable std::unordered_set<number_t> running_programs;

};
