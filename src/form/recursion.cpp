#include "form/recursion.hpp"

#include <algorithm>
#include <set>

#include "form/formula_util.hpp"
#include "form/function.hpp"

// Recursively traverses the expression tree to find recursion depth
static void findDepthRecursive(const Expression& e,
                               const std::string& func_name,
                               int64_t& max_depth) {
  if (e.type == Expression::Type::FUNCTION && e.name == func_name) {
    if (e.children.size() == 1) {
      const auto& arg = e.children[0];
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
  for (const auto& child : e.children) {
    findDepthRecursive(child, func_name, max_depth);
  }
}

// Validate recursive call structure: forbid self-reference a(n) and restrict
// arguments to n-k with k > 0.
static bool checkRecursiveCalls(const Expression& expr,
                                const std::string& func_name,
                                std::string& error_msg) {
  if (expr.type == Expression::Type::FUNCTION && expr.name == func_name) {
    if (expr.children.size() == 1) {
      const auto& arg = expr.children[0];

      // Reject direct self-reference func_name(n)
      if (arg.type == Expression::Type::PARAMETER && arg.name == "n") {
        error_msg = "RHS contains " + func_name + "(n) self-reference";
        return false;
      }

      // Allow only n + constant with negative constant (i.e., n-k)
      if (arg.type == Expression::Type::SUM && arg.children.size() == 2) {
        const auto& left = arg.children[0];
        const auto& right = arg.children[1];

        if (left.type == Expression::Type::PARAMETER && left.name == "n" &&
            right.type == Expression::Type::CONSTANT) {
          int64_t offset = right.value.asInt();
          if (offset >= 0) {
            error_msg = "RHS contains " + func_name + "(" + arg.toString() +
                        ") with non-decreasing offset";
            return false;
          }
        } else {
          error_msg = "RHS contains " + func_name + "(" + arg.toString() +
                      ") with invalid argument";
          return false;
        }
      } else if (arg.type != Expression::Type::SUM) {
        error_msg = "RHS contains " + func_name + "(" + arg.toString() +
                    ") with non-standard form";
        return false;
      }
    }
  }

  for (const auto& child : expr.children) {
    if (!checkRecursiveCalls(child, func_name, error_msg)) {
      return false;
    }
  }
  return true;
}

bool isRecursive(const Formula& formula, const std::string& func_name,
                 Expression::Type type) {
  auto deps = FormulaUtil::getDependencies(formula, type, false, false);
  for (const auto& dep : deps) {
    if (dep.first == func_name && dep.second == func_name) {
      return true;
    }
  }
  return false;
}

// Helper function to check if a function depends on another
static bool dependsOn(const std::multimap<std::string, std::string>& deps,
                      const std::string& from, const std::string& to) {
  auto range = deps.equal_range(from);
  for (auto it = range.first; it != range.second; ++it) {
    if (it->second == to) {
      return true;
    }
  }
  return false;
}

bool hasMutualRecursion(const Formula& formula, Expression::Type type) {
  // Get transitive dependencies between functions
  auto deps = FormulaUtil::getDependencies(formula, type, true, false);

  // Build a set of all function names
  std::set<std::string> funcNames;
  for (const auto& e : formula.entries) {
    if (e.first.type == type && !e.first.name.empty()) {
      funcNames.insert(e.first.name);
    }
  }

  // Check for mutual recursion: A depends on B and B depends on A (where A !=
  // B), and neither A nor B is self-recursive.
  // If at least one function in the cycle is self-recursive, the LEAN code
  // generator will use Nat domain with pattern offsets, which can prove
  // termination.
  for (auto itA = funcNames.begin(); itA != funcNames.end(); ++itA) {
    for (auto itB = std::next(itA); itB != funcNames.end(); ++itB) {
      const auto& funcA = *itA;
      const auto& funcB = *itB;

      // Check for mutual dependency (A->B and B->A)
      if (dependsOn(deps, funcA, funcB) && dependsOn(deps, funcB, funcA)) {
        // Check if either A or B is self-recursive
        // Use non-transitive check for self-recursion
        bool aIsSelfRecursive = isRecursive(formula, funcA, type);
        bool bIsSelfRecursive = isRecursive(formula, funcB, type);

        // If neither is self-recursive, LEAN can't prove termination
        // because the domain will be Int instead of Nat
        if (!aIsSelfRecursive && !bIsSelfRecursive) {
          return true;
        }
      }
    }
  }
  return false;
}

// Verify that initial_terms contain a contiguous block of size max_depth
// starting at the smallest provided index. Returns false and sets error_msg on
// failure.
static bool checkInitialTerms(
    const std::string& func_name,
    const std::map<int64_t, Expression>& initial_terms, int64_t max_depth,
    std::string& error_msg) {
  if (max_depth <= 0) {
    return true;
  }
  if (initial_terms.empty()) {
    error_msg = func_name + "(n) has no initial terms (requires " +
                std::to_string(max_depth) + ")";
    return false;
  }

  const auto min_index =
      std::min_element(
          initial_terms.begin(), initial_terms.end(),
          [](const auto& a, const auto& b) { return a.first < b.first; })
          ->first;
  const int64_t start_index = min_index;
  for (int64_t i = 0; i < max_depth; i++) {
    const auto expected_index = start_index + i;
    if (initial_terms.find(expected_index) == initial_terms.end()) {
      error_msg = func_name + "(n) is missing initial term for index " +
                  std::to_string(expected_index) + " (required " +
                  std::to_string(max_depth) + " initial terms, " + "but only " +
                  std::to_string(initial_terms.size()) + " provided)";
      return false;
    }
  }

  return true;
}

bool validateRecursiveFunction(const Function& func, std::string& error_msg) {
  // Extract function components
  const auto& func_name = func.name;
  const auto& recursive_rhs = func.general_case;
  const auto& initial_terms = func.base_cases;

  bool is_valid = true;

  // Check if RHS contains recursive calls to this function
  bool has_recursive_calls =
      recursive_rhs.contains(Expression::Type::FUNCTION, func_name);

  // 1. Check if RHS contains func_name(n) (invalid self-reference) or invalid
  // argument forms
  if (has_recursive_calls) {
    is_valid = checkRecursiveCalls(recursive_rhs, func_name, error_msg);
  }

  // 2. Check if number of initial terms is sufficient
  if (is_valid && has_recursive_calls) {
    // Find the maximum recursion depth
    int64_t max_depth = 0;
    findDepthRecursive(recursive_rhs, func_name, max_depth);

    is_valid =
        checkInitialTerms(func_name, initial_terms, max_depth, error_msg);
  }

  return is_valid;
}
