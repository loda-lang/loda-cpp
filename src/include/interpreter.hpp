#pragma once

#include "memory.hpp"
#include "program.hpp"
#include "util.hpp"

#include <unordered_map>
#include <unordered_set>

struct pair_hasher
{
  std::size_t operator()( const std::pair<int64_t, Number> &p ) const
  {
    return (p.first << 32) ^ p.second.hash();
  }
};

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  static Number calc( const Operation::Type type, const Number& target, const Number& source );

  size_t run( const Program &p, Memory &mem );

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

  // TODO: avoid this friend class
  friend class Evaluator;

};
