#pragma once

#include <cstddef>

// Maximum number of terms allowed in a formula or variant definition
// This prevents formulas from growing exponentially complex (e.g., A042269 with 2768 terms)
// Set to 500 to allow legitimate complex formulas like A006430 (256 terms) while
// catching pathological cases
constexpr size_t MAX_FORMULA_TERMS = 500;
