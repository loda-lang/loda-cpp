#include "form/function.hpp"

#include "form/formula.hpp"

Function::Function(const std::string& name, const Expression& general_case,
                   const std::map<int64_t, Expression>& base_cases)
    : name(name),
      general_case(general_case),
      base_cases(base_cases),
      has_general_case(true) {}

Function::Function(const std::string& name)
    : name(name),
      general_case(Expression::Type::CONSTANT),
      base_cases(),
      has_general_case(false) {}

std::vector<Function> Function::fromFormula(const Formula& formula,
                                            const std::string& name) {
  // Map from function name to its general case and base cases
  std::map<std::string, Expression> general_cases;
  std::map<std::string, std::map<int64_t, Expression>> all_base_cases;

  // Iterate through all entries in the formula
  for (const auto& entry : formula.entries) {
    const auto& lhs = entry.first;
    const auto& rhs = entry.second;

    // Only process function definitions
    if (lhs.type != Expression::Type::FUNCTION || lhs.name.empty()) {
      continue;
    }

    // If a specific name is requested, skip functions that don't match
    if (!name.empty() && lhs.name != name) {
      continue;
    }

    // Skip functions with incorrect arity
    if (lhs.children.size() != 1) {
      continue;
    }

    const auto& arg = lhs.children[0];
    const auto& func_name = lhs.name;

    // Check if this is a general case: func(n)
    if (arg.type == Expression::Type::PARAMETER && arg.name == "n") {
      general_cases[func_name] = rhs;
    }
    // Check if this is a base case: func(constant)
    else if (arg.type == Expression::Type::CONSTANT) {
      all_base_cases[func_name][arg.value.asInt()] = lhs;
    }
  }

  // Build the list of functions
  std::vector<Function> functions;
  for (const auto& entry : general_cases) {
    const auto& func_name = entry.first;
    const auto& general_case = entry.second;
    const auto& base_cases = all_base_cases[func_name];
    functions.emplace_back(func_name, general_case, base_cases);
  }

  // Add functions that only have base cases (no general case)
  for (const auto& entry : all_base_cases) {
    const auto& func_name = entry.first;
    if (general_cases.find(func_name) == general_cases.end()) {
      functions.emplace_back(func_name);
      functions.back().base_cases = entry.second;
    }
  }

  return functions;
}

int64_t Function::getMinimumBaseCase() const {
  int64_t minBaseCase = -1;
  for (const auto& entry : base_cases) {
    if (minBaseCase == -1 || entry.first < minBaseCase) {
      minBaseCase = entry.first;
    }
  }
  return minBaseCase;
}
