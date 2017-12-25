#pragma once

#include "types.hpp"

#include <array>
#include <iostream>
#include <vector>

class Memory
{
public:

  Memory();

  bool IsLessThan( const Memory& other, const std::vector<var_t> cmp_vars );

  std::array<value_t, 256> regs;

};

std::ostream& operator<<( std::ostream& out, const Memory& m );
