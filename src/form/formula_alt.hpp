#pragma once

#include <map>

#include "form/formula.hpp"

typedef std::multimap<Expression, Expression> Alternatives;

bool findAlternativesByResolve(Alternatives& alt);

bool applyAlternatives(const Alternatives& alt, Formula& formula);

bool simplifyFormulaUsingAlternatives(Formula& formula);
