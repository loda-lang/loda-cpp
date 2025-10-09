#include "form/formula_util.hpp"

#include <set>

#include "form/expression_util.hpp"

void FormulaUtil::resolveIdentities(Formula& formula) {
  auto copy = formula.entries;
  for (auto& e : copy) {
    if (ExpressionUtil::isSimpleFunction(e.first) &&
        ExpressionUtil::isSimpleFunction(e.second) &&
        copy.find(e.second) != copy.end()) {
      formula.entries.erase(e.first);
      formula.replaceName(e.second.name, e.first.name);
    }
  }
}

void replaceFunction(Expression& target, const std::string& func,
                     const Expression& param, const Expression& val) {
  // bottom-up
  for (auto& c : target.children) {
    replaceFunction(c, func, param, val);
  }
  ExpressionUtil::normalize(target);
  if (target.type != Expression::Type::FUNCTION ||
      target.children.size() != 1 || target.name != func) {
    return;
  }
  auto updated = val;
  updated.replaceAll(param, target.children.front());
  ExpressionUtil::normalize(updated);
  target = updated;
}

void FormulaUtil::resolveSimpleFunctions(Formula& formula) {
  // collect function definitions
  std::set<std::string> simple_funcs;
  std::map<std::string, Expression> params, defs;
  for (auto& e : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(e.first)) {
      simple_funcs.insert(e.first.name);
      params[e.first.name] = e.first.children.front();
      defs[e.first.name] = e.second;
    }
  }
  // filter out non-simple functions
  auto deps =
      getDependencies(formula, Expression::Type::FUNCTION, false, false);
  for (auto& e : formula.entries) {
    if (e.first.type != Expression::Type::FUNCTION) {
      continue;  // should not happen
    }
    auto f = e.first.name;
    bool is_simple = true;
    if (!ExpressionUtil::isSimpleFunction(e.first)) {
      is_simple = false;
    }
    for (auto it : deps) {
      auto functions = getDefinitions(formula);
      if (it.first == f && std::find(functions.begin(), functions.end(),
                                     it.second) != functions.end()) {
        is_simple = false;
        break;
      }
    }
    if (!is_simple) {
      simple_funcs.erase(e.first.name);
    }
  }
  // perform replacements
  for (const auto& f : simple_funcs) {
    for (auto& e : formula.entries) {
      replaceFunction(e.second, f, params[f], defs[f]);
    }
  }
}

void FormulaUtil::resolveSimpleRecursions(Formula& formula) {
  // collect functions
  std::set<std::string> funcs;
  for (auto& e : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(e.first)) {
      funcs.insert(e.first.name);
    }
  }
  // collect and check their slopes and offsets
  std::map<std::string, Number> slopes, offsets;
  std::map<std::string, Expression> params;
  std::map<Number, Number> constants;
  for (auto& f : funcs) {
    constants.clear();
    bool found_slope = false;
    Number slope = 0;
    for (auto& e : formula.entries) {
      if (e.first.type != Expression::Type::FUNCTION) {
        continue;
      }
      if (e.first.name != f) {
        continue;
      }
      auto arg_type = e.first.children.front().type;
      if (arg_type == Expression::Type::CONSTANT) {
        if (e.second.type != Expression::Type::CONSTANT) {
          constants.clear();
          break;
        }
        constants[e.first.children.front().value] = e.second.value;
      } else if (arg_type == Expression::Type::PARAMETER) {
        params[f] = e.first.children.front();
        auto val = e.second;
        if (val.type != Expression::Type::SUM) {
          found_slope = false;
          break;
        }
        if (val.children.size() != 2) {
          found_slope = false;
          break;
        }
        if (val.children.at(1).type != Expression::Type::CONSTANT) {
          found_slope = false;
          break;
        }
        Expression predecessor(
            Expression::Type::SUM, "",
            {params[f],
             Expression(Expression::Type::CONSTANT, "", Number(-1))});
        Expression prevTerm(Expression::Type::FUNCTION, f, {predecessor});
        if (val.children.at(0) != prevTerm) {
          found_slope = false;
          break;
        }
        slope = val.children.at(1).value;
        found_slope = true;
      } else {
        found_slope = false;
        break;
      }
    }
    if (!found_slope || constants.find(Number::ZERO) == constants.end()) {
      continue;
    }
    auto offset = constants.at(Number::ZERO);
    for (const auto& c : constants) {
      auto expected_val = slope;
      expected_val *= c.first;
      expected_val += offset;
      if (c.second != expected_val) {
        found_slope = false;
        break;
      }
    }
    if (found_slope) {
      slopes[f] = slope;
      offsets[f] = offset;
    }
  }
  for (auto& f : funcs) {
    if (slopes.find(f) == slopes.end()) {
      continue;
    }
    // remove function
    auto it = formula.entries.begin();
    while (it != formula.entries.end()) {
      if (it->first.name == f) {
        it = formula.entries.erase(it);
      } else {
        it++;
      }
    }
    // add simple function
    Expression prod(
        Expression::Type::PRODUCT, "",
        {Expression(Expression::Type::CONSTANT, "", slopes[f]), params[f]});
    Expression sum(
        Expression::Type::SUM, "",
        {Expression(Expression::Type::CONSTANT, "", offsets[f]), prod});
    ExpressionUtil::normalize(sum);
    Expression func(Expression::Type::FUNCTION, f, {params[f]});
    formula.entries[func] = sum;
  }
}

std::vector<std::string> FormulaUtil::getDefinitions(
    const Formula& formula, const Expression::Type type,
    bool sortByDependencies) {
  std::vector<std::string> result;
  for (const auto& e : formula.entries) {
    if (e.first.type == type &&
        std::find(result.begin(), result.end(), e.first.name) == result.end()) {
      result.push_back(e.first.name);
    }
  }
  if (sortByDependencies) {
    const auto deps = getDependencies(formula, type, true, true);
    std::sort(result.begin(), result.end(),
              [&](const std::string& a, const std::string& b) {
                auto depsA = deps.equal_range(a);
                return !std::any_of(
                    depsA.first, depsA.second,
                    [&](const std::pair<std::string, std::string>& e) {
                      return e.second == b;
                    });
              });
  } else {
    std::sort(result.begin(), result.end());
  }
  return result;
}

bool containsPair(std::multimap<std::string, std::string>& deps,
                  const std::string& key, const std::string& value) {
  auto range = deps.equal_range(key);
  for (auto i = range.first; i != range.second; ++i) {
    if (i->second == value) {
      return true;
    }
  }
  return false;
}

void collectDeps(const std::string& fname, const Expression& e,
                 Expression::Type type,
                 std::multimap<std::string, std::string>& deps) {
  if (e.type == type && !e.name.empty() && !containsPair(deps, fname, e.name)) {
    deps.insert({fname, e.name});
  }
  for (const auto& c : e.children) {
    collectDeps(fname, c, type, deps);
  }
}

std::pair<std::string, std::string> findMissingPair(
    std::multimap<std::string, std::string>& deps) {
  std::pair<std::string, std::string> result;
  for (auto i : deps) {
    auto range = deps.equal_range(i.second);
    for (auto j = range.first; j != range.second; ++j) {
      if (!containsPair(deps, i.first, j->second)) {
        result = {i.first, j->second};
        return result;
      }
    }
  }
  return result;
}

std::multimap<std::string, std::string> FormulaUtil::getDependencies(
    const Formula& formula, const Expression::Type type, bool transitive,
    bool ignoreSelf) {
  std::multimap<std::string, std::string> deps;
  for (auto& e : formula.entries) {
    if (e.first.type == type && !e.first.name.empty()) {
      collectDeps(e.first.name, e.second, type, deps);
    }
  }
  if (transitive) {
    while (true) {
      auto missing = findMissingPair(deps);
      if (missing.first.empty()) {
        break;
      }
      deps.insert(missing);
    }
  }
  if (ignoreSelf) {
    auto it = deps.begin();
    while (it != deps.end()) {
      if (it->first == it->second) {
        it = deps.erase(it);
      } else {
        it++;
      }
    }
  }
  return deps;
}

bool FormulaUtil::isRecursive(const Formula& formula,
                              const std::string& funcName,
                              Expression::Type type) {
  auto deps = getDependencies(formula, type, false, false);
  for (auto it : deps) {
    if (it.first == funcName && it.second == funcName) {
      return true;
    }
  }
  return false;
}

void FormulaUtil::convertInitialTermsToIf(Formula& formula,
                                          const Expression::Type type) {
  auto it = formula.entries.begin();
  while (it != formula.entries.end()) {
    auto left = it->first;
    auto general = ExpressionUtil::newFunction(left.name);
    general.type = type;
    auto general_it = formula.entries.find(general);
    if (ExpressionUtil::isInitialTerm(left) &&
        general_it != formula.entries.end()) {
      auto index_expr = left.children.front();
      general_it->second =
          Expression(Expression::Type::IF, "",
                     {index_expr, it->second, general_it->second});
      it = formula.entries.erase(it);
    } else {
      it++;
    }
  }
}

bool FormulaUtil::extractArgumentOffset(const Expression& arg, Number& offset) {
  if (arg.type == Expression::Type::PARAMETER) {
    offset = Number::ZERO;
    return true;
  } else if (arg.type == Expression::Type::SUM && arg.children.size() == 2 &&
             arg.children[0].type == Expression::Type::PARAMETER &&
             arg.children[1].type == Expression::Type::CONSTANT) {
    offset = arg.children[1].value;
    return true;
  }
  return false;
}

void FormulaUtil::removeFunctionEntries(Formula& formula,
                                        const std::string& funcName) {
  auto entryIt = formula.entries.begin();
  while (entryIt != formula.entries.end()) {
    if (entryIt->first.type == Expression::Type::FUNCTION &&
        entryIt->first.name == funcName) {
      entryIt = formula.entries.erase(entryIt);
    } else {
      entryIt++;
    }
  }
}

namespace {

// Helper function to check if a function is a simple recursive reference
bool isSimpleRecursiveReference(const Formula& formula,
                                const std::string& funcName,
                                const Expression& rhs,
                                const std::set<std::string>& processedFuncs,
                                std::string& refFuncName,
                                Number& offset) {
  // Check if RHS is a simple function call
  if (rhs.type != Expression::Type::FUNCTION || rhs.children.size() != 1) {
    return false;
  }
  
  refFuncName = rhs.name;
  const auto& arg = rhs.children.front();
  
  // Skip if the referenced function was already processed
  if (processedFuncs.find(refFuncName) != processedFuncs.end()) {
    return false;
  }
  
  // Extract offset
  if (!FormulaUtil::extractArgumentOffset(arg, offset)) {
    return false;
  }
  
  // Check if the referenced function is recursive
  if (!FormulaUtil::isRecursive(formula, refFuncName)) {
    return false;
  }
  
  // Check if there are no other dependencies
  auto deps = FormulaUtil::getDependencies(formula, Expression::Type::FUNCTION, false, false);
  for (const auto& dep : deps) {
    if (dep.first == funcName && dep.second != refFuncName) {
      return false;  // Has other dependencies
    }
  }
  
  return true;
}

// Helper function to adjust index by offset
void adjustIndexByOffset(Expression& expr, const Number& offset) {
  if (expr.children.size() != 1) {
    return;
  }
  
  auto& arg = expr.children.front();
  if (arg.type == Expression::Type::CONSTANT) {
    // Initial term: adjust constant index
    arg.value -= offset;
  } else if (arg.type == Expression::Type::SUM &&
             arg.children.size() == 2 &&
             arg.children[0].type == Expression::Type::PARAMETER &&
             arg.children[1].type == Expression::Type::CONSTANT) {
    // Already has an offset on the left side, adjust it
    arg.children[1].value -= offset;
    ExpressionUtil::normalize(expr.children[0]);
  }
  // For PARAMETER type, no adjustment needed for the left side
}

// Helper function to perform the replacement
void performReplacement(Formula& formula,
                       const std::string& funcName,
                       const std::string& refFuncName,
                       const Number& offset,
                       const std::map<Expression, Expression>& refFuncEntries) {
  // For each entry of the referenced function
  for (const auto& refEntry : refFuncEntries) {
    Expression newLeft = refEntry.first;
    newLeft.name = funcName;
    
    // Adjust the index by subtracting the offset
    adjustIndexByOffset(newLeft, offset);
    
    // Replace references to refFuncName with funcName in the RHS
    Expression newRight = refEntry.second;
    newRight.replaceName(refFuncName, funcName);
    
    formula.entries[newLeft] = newRight;
  }
  
  // Replace all references to refFuncName with funcName in remaining formulas
  for (auto& entry : formula.entries) {
    entry.second.replaceName(refFuncName, funcName);
  }
  
  // Remove all entries of the referenced function
  FormulaUtil::removeFunctionEntries(formula, refFuncName);
}

}  // namespace

void FormulaUtil::replaceSimpleRecursiveReferences(Formula& formula) {
  // Find all functions
  auto funcs = getDefinitions(formula);
  
  // Track which functions were created by this algorithm
  std::set<std::string> processedRecursiveFuncs;
  
  // For each function, check if it's a simple reference to another recursive function
  for (const auto& funcName : funcs) {
    // Skip if funcName is an external OEIS sequence
    if (!funcName.empty() && std::isupper(funcName[0])) {
      continue;
    }
    
    // Get the general definition (the one with a parameter, not a constant)
    Expression funcExpr = ExpressionUtil::newFunction(funcName);
    auto it = formula.entries.find(funcExpr);
    if (it == formula.entries.end()) {
      continue;  // No general definition found
    }
    
    // Check if this is a simple recursive reference
    std::string refFuncName;
    Number offset;
    if (!isSimpleRecursiveReference(formula, funcName, it->second,
                                    processedRecursiveFuncs, refFuncName, offset)) {
      continue;
    }
    
    // Collect all entries for the referenced function
    auto refFuncEntries = formula.collectFunctionEntries(refFuncName);
    
    // Check if simplification would create negative initial term indices
    bool hasNegativeIndices = false;
    for (const auto& entry : refFuncEntries) {
      if (entry.first.children.size() == 1 &&
          entry.first.children.front().type == Expression::Type::CONSTANT) {
        Number adjustedIndex = entry.first.children.front().value;
        adjustedIndex -= offset;
        if (adjustedIndex < Number::ZERO) {
          hasNegativeIndices = true;
          break;
        }
      }
    }
    if (hasNegativeIndices) {
      continue;  // Skip simplification to avoid negative indices
    }
    
    // Remove the simple reference definition
    formula.entries.erase(it);
    
    // Perform the replacement
    performReplacement(formula, funcName, refFuncName, offset, refFuncEntries);
    
    // Mark this function as processed
    processedRecursiveFuncs.insert(funcName);
  }
}
