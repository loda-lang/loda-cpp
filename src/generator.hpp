#pragma once

#include "machine.hpp"
#include "program.hpp"

class Generator
{
public:

  Program::UPtr Generate( Machine& m );

};
