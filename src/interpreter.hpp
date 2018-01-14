#pragma once

#include "program.hpp"
#include "sequence.hpp"
#include "value.hpp"

class Interpreter
{
public:

  bool Run( Program& p, Sequence& mem );

  Value Get( Operand a, const Sequence& mem, bool get_address = false );

  void Set( Operand a, Value v, Sequence& mem );

  bool IsLessThan( const Sequence& m1, const Sequence& m2, const std::vector<Operand>& cmp_vars );

};
