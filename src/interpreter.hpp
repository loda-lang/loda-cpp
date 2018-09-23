#pragma once

#include "common.hpp"
#include "program.hpp"

class Interpreter
{
public:

  bool run( const Program& p, Sequence& mem );

  Sequence eval( const Program& p, number_t length );

private:

  number_t get( Operand a, const Sequence& mem, bool get_address = false );

  void set( Operand a, number_t v, Sequence& mem );

  bool isLessThan( const Sequence& m1, const Sequence& m2, const std::vector<Operand>& cmp_vars );

};
