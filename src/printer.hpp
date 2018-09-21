#pragma once

#include "program.hpp"

#include <iostream>

class Printer
{
public:

  void Print( const Program& p, std::ostream& out );

  void Print( const Operation& op, std::ostream& out, int indent = 0 );

};
