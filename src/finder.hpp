#pragma once

#include "program.hpp"
#include "sequence.hpp"

class Finder
{
public:

  Program::UPtr Find( const Sequence& target );

};
