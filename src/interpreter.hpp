#pragma once

#include "program.hpp"
#include "memory.hpp"
#include "value.hpp"

class Interpreter
{
public:

  bool Run( Program& p, Memory& mem );

  Value Get( Operand a, const Memory& mem );

  void Set( Operand a, Value v, Memory& mem );

  bool IsLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmp_vars );

};
