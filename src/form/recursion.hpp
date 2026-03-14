#pragma once

#include <map>
#include <string>

#include "form/formula.hpp"
#include "form/function.hpp"

/**
 * Validates a single recursive function definition.
 *
 * Checks if a recursive formula is syntactically valid by verifying:
 * 1. No self-reference: func.name(n) must not appear in RHS
 * 2. Valid recursion form: only func.name(n-k) for positive constant k allowed
 * 3. Sufficient initial terms: if max recursion depth is d, requires initial
 *    terms for indices starting at the minimum provided initial term
 *
 * @param func The Function to validate
 * @param error_msg Output parameter for error message if validation fails
 * @return true if the recursive formula is valid, false otherwise
 */
bool validateRecursiveFunction(const Function& func, std::string& error_msg);

// Returns true if the function is self-recursive.
bool isRecursive(const Formula& formula, const std::string& func_name,
                 Expression::Type type = Expression::Type::FUNCTION);

// Returns true if there is a mutual recursion cycle (A <-> B) where neither
// side is self-recursive.
bool hasMutualRecursion(const Formula& formula,
                        Expression::Type type = Expression::Type::FUNCTION);
