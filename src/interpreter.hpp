#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Interpreter
{
public:

  Interpreter( const Settings& settings );

  bool run( const Program& p, Memory& mem );

  Sequence eval( const Program& p );

private:

  number_t get( Operand a, const Memory& mem, bool getAddress = false );

  void set( Operand a, number_t v, Memory& mem );

  bool isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmpVars );

  const Settings& settings;

};
