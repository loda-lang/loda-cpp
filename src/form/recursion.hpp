#pragma once

#include <map>
#include <string>

#include "form/expression.hpp"

/**
 * Validates a single recursive formula definition.
 *
 * Checks if a recursive formula is syntactically valid by verifying:
 * 1. No self-reference: func_name(n) must not appear in RHS
 * 2. Valid recursion form: only func_name(n-k) for positive constant k allowed
 * 3. Sufficient initial terms: if max recursion depth is d, requires initial
 *    terms for indices 0..d-1
 *
 * @param func_name The name of the function being validated (e.g., "a", "b")
 * @param recursive_rhs The right-hand side expression of the recursive
 * definition
 * @param initial_terms Map from index to LHS expression for initial terms
 * @param error_msg Output parameter for error message if validation fails
 * @return true if the recursive formula is valid, false otherwise
 */
bool validateRecursiveFormula(
    const std::string& func_name, const Expression& recursive_rhs,
    const std::map<int64_t, Expression>& initial_terms, std::string& error_msg);

// Extract the recursive definition (rhs) and initial terms for a function.
// Returns true if a recursive definition func_name(n) was found.
bool extractRecursiveDefinition(
    const std::vector<std::pair<Expression, Expression>>& entries,
    const std::string& func_name, Expression& recursive_rhs,
    std::map<int64_t, Expression>& initial_terms);
