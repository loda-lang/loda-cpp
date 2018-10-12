#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Interpreter
{
public:

  Interpreter( const Settings& settings );

  bool run( const Program& p, Memory& mem ) const;

  Sequence eval( const Program& p, int num_terms = -1 ) const;

private:

  number_t get( Operand a, const Memory& mem, bool getAddress = false ) const;

  void set( Operand a, number_t v, Memory& mem ) const;

  bool isLessThan( const Memory& m1, const Memory& m2, const std::vector<Operand>& cmpVars ) const;

  Memory getLoopFragment( const Memory& mem, const Operation& op ) const;

  const Settings& settings;

  const bool is_debug;

};
