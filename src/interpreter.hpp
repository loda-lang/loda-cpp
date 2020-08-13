#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  size_t run( const Program &p, Memory &mem ) const;

  size_t eval( const Program &p, Sequence &seq, int num_terms = -1 ) const;

  size_t eval( const Program &p, std::vector<Sequence> &seqs, int num_terms = -1 ) const;

  bool check( const Program &p, const Sequence &expected_seq ) const;

private:

  number_t get( Operand a, const Memory &mem, bool get_address = false ) const;

  void set( Operand a, number_t v, Memory &mem ) const;

  const Settings &settings;

  const bool is_debug;

};
