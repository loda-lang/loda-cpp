#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  bool run( const Program &p, Memory &mem ) const;

  void eval( const Program &p, Sequence &seq, int num_terms = -1 ) const;

  void eval( const Program &p, std::vector<Sequence> &seqs, int num_terms = -1 ) const;

private:

  number_t get( Operand a, const Memory &mem, bool get_address = false ) const;

  void set( Operand a, number_t v, Memory &mem ) const;

  bool isLessThan( const Memory &m1, const Memory &m2, const std::vector<Operand> &cmp_vars ) const;

  const Settings &settings;

  const bool is_debug;

};
