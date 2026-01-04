#include "form/recursion.hpp"

#include <functional>

bool validateRecursiveFormula(
    const std::string& func_name,
    const Expression& recursive_rhs,
    const std::map<int64_t, Expression>& initial_terms,
    std::string& error_msg) {
  
  bool is_valid = true;

  // Check if RHS contains recursive calls to this function
  bool has_recursive_calls = recursive_rhs.contains(Expression::Type::FUNCTION, func_name);

  // 1. Check if RHS contains func_name(n) (invalid self-reference) or invalid argument forms
  if (has_recursive_calls) {
    // Check all function calls in RHS
    std::function<bool(const Expression&)> check_recursive_calls;
    check_recursive_calls = [&](const Expression& expr) -> bool {
      if (expr.type == Expression::Type::FUNCTION && expr.name == func_name) {
        if (expr.children.size() == 1) {
          const auto& arg = expr.children[0];
          
          // Check if argument is exactly 'n' (invalid self-reference)
          if (arg.type == Expression::Type::PARAMETER && arg.name == "n") {
            is_valid = false;
            error_msg = "RHS contains " + func_name + "(n) self-reference";
            return false;
          }

          // Check if argument is of the form n+k where k is a constant
          // Valid forms: n-1, n-2, etc. (represented as n+(-1), n+(-2))
          // Invalid forms: n+1, 2*n, etc.
          if (arg.type == Expression::Type::SUM && arg.children.size() == 2) {
            const auto& left = arg.children[0];
            const auto& right = arg.children[1];
            
            // Check for n + constant pattern
            if (left.type == Expression::Type::PARAMETER && left.name == "n" &&
                right.type == Expression::Type::CONSTANT) {
              int64_t offset = right.value.asInt();
              // Valid if offset is negative (n + negative = n - positive)
              if (offset >= 0) {
                is_valid = false;
                error_msg = "RHS contains " + func_name + "(" + arg.toString() + 
                           ") with non-decreasing offset";
                return false;
              }
            } else {
              // Some other expression form - invalid
              is_valid = false;
              error_msg = "RHS contains " + func_name + "(" + arg.toString() + 
                         ") with invalid argument";
              return false;
            }
          } else if (arg.type != Expression::Type::SUM) {
            // Not a sum expression - invalid (only n+constant form is allowed)
            is_valid = false;
            error_msg = "RHS contains " + func_name + "(" + arg.toString() + 
                       ") with non-standard form";
            return false;
          }
        }
      }

      // Recursively check children
      for (const auto& child : expr.children) {
        if (!check_recursive_calls(child)) {
          return false;
        }
      }
      return true;
    };

    check_recursive_calls(recursive_rhs);
  }

  // 2. Check if number of initial terms is sufficient
  if (is_valid && has_recursive_calls) {
    // Find the maximum recursion depth
    int64_t max_depth = 0;
    std::function<void(const Expression&)> find_max_depth;
    find_max_depth = [&](const Expression& expr) {
      if (expr.type == Expression::Type::FUNCTION && expr.name == func_name) {
        if (expr.children.size() == 1) {
          const auto& arg = expr.children[0];
          if (arg.type == Expression::Type::SUM && arg.children.size() == 2) {
            const auto& left = arg.children[0];
            const auto& right = arg.children[1];
            if (left.type == Expression::Type::PARAMETER && left.name == "n" &&
                right.type == Expression::Type::CONSTANT) {
              int64_t offset = right.value.asInt();
              int64_t depth = -offset;  // Convert n+(-k) to depth k
              if (depth > max_depth) {
                max_depth = depth;
              }
            }
          }
        }
      }
      for (const auto& child : expr.children) {
        find_max_depth(child);
      }
    };

    find_max_depth(recursive_rhs);

    // Check if we have initial terms for indices 0 through max_depth-1
    if (max_depth > 0) {
      for (int64_t i = 0; i < max_depth; i++) {
        if (initial_terms.find(i) == initial_terms.end()) {
          is_valid = false;
          error_msg = "Function " + func_name + " missing initial term for index " + std::to_string(i) + 
                     " (required " + std::to_string(max_depth) + " initial terms, " +
                     "but only " + std::to_string(initial_terms.size()) + " provided)";
          break;
        }
      }
    }
  }

  return is_valid;
}
