#pragma once

#include "value.hpp"

#include <array>
#include <iostream>
#include <vector>

class Memory
{
public:

  Memory();

  bool IsLessThan( const Memory& other, const std::vector<Value> cmp_vars );

  std::array<Value, 256> regs;

};

std::ostream& operator<<( std::ostream& out, const Memory& m );
