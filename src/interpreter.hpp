#pragma once

#include "common.hpp"
#include "program.hpp"

class Interpreter
{
public:

  bool Run( const Program& p, Sequence& mem );

  Sequence Eval( const Program& p, number_t length );

private:

  number_t Get( Operand a, const Sequence& mem, bool get_address = false );

  void Set( Operand a, number_t v, Sequence& mem );

  bool IsLessThan( const Sequence& m1, const Sequence& m2, const std::vector<Operand>& cmp_vars );

};
