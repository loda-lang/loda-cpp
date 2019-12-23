#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Interpreter
{
public:

  Interpreter( const Settings &settings );

  bool run( const Program &p, Memory &mem ) const;

  Sequence eval( const Program &p, int num_terms = -1 ) const;

  static number_t pow( number_t base, number_t exp );

  static number_t gcd( number_t a, number_t b );

private:

  number_t get( Operand a, const Memory &mem, bool get_address = false ) const;

  void set( Operand a, number_t v, Memory &mem ) const;

  bool isLessThan( const Memory &m1, const Memory &m2, const std::vector<Operand> &cmp_vars ) const;

  const Settings &settings;

  const bool is_debug;

};
