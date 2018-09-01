#pragma once

#include "program.hpp"

#include <iostream>

class Printer
{
public:

  void Print( Program& p, std::ostream& out );

  void Print( Operation& op, std::ostream& out, int indent = 0 );

};
