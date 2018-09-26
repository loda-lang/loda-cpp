#pragma once

#include "number.hpp"
#include "program.hpp"

class Interpreter
{
public:

  bool run( const Program& p, Memory& mem );

  Sequence eval( const Program& p, number_t length );

private:

  number_t get( Operand a, const Memory& mem, bool get_address = false );

  void set( Operand a, number_t v, Memory& mem );

  bool isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmp_vars );

};
