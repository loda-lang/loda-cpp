#pragma once

#include <map>

#include "form/formula.hpp"

typedef std::multimap<Expression, Expression> Alternatives;

bool findAlternatives(Alternatives& alt);

bool applyAlternatives(const Alternatives& alt, Formula& f);
