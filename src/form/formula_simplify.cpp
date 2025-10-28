#include "form/formula_simplify.hpp"

#include <map>
#include <set>
#include <string>

#include "form/expression_util.hpp"
#include "form/formula_util.hpp"

void FormulaSimplify::resolveIdentities(Formula& formula) {
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

void FormulaSimplify::resolveSimpleFunctions(Formula& formula) {
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
  auto deps = FormulaUtil::getDependencies(formula, Expression::Type::FUNCTION,
                                           false, false);
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
      auto functions = FormulaUtil::getDefinitions(formula);
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

void FormulaSimplify::replaceTrivialRecursions(Formula& formula) {
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

bool extractArgumentOffset(const Expression& arg, Number& offset) {
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

// Helper function to check if expression contains parameter outside of function
// calls
bool containsParameterOutsideFunction(const Expression& expr,
                                      const std::string& funcName) {
  if (expr.type == Expression::Type::PARAMETER) {
    return true;
  }
  if (expr.type == Expression::Type::FUNCTION && expr.name == funcName) {
    return false;  // Don't recurse into function calls
  }
  for (const auto& child : expr.children) {
    if (containsParameterOutsideFunction(child, funcName)) {
      return true;
    }
  }
  return false;
}

// Helper function to check if a function is a simple recursive reference
bool isSimpleRecursiveReference(const Formula& formula,
                                const std::string& funcName,
                                const Expression& rhs,
                                const std::set<std::string>& processedFuncs,
                                std::string& refFuncName, Number& offset) {
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
  if (!extractArgumentOffset(arg, offset)) {
    return false;
  }

  // Check if the referenced function is recursive
  if (!FormulaUtil::isRecursive(formula, refFuncName)) {
    return false;
  }

  // Check if the referenced function's RHS contains parameters (n) outside of
  // function calls If so, skip simplification as it would create incorrect
  // formulas
  Expression refFuncExpr = ExpressionUtil::newFunction(refFuncName);
  auto it = formula.entries.find(refFuncExpr);
  if (it != formula.entries.end()) {
    if (containsParameterOutsideFunction(it->second, refFuncName)) {
      // RHS contains parameter outside function calls, cannot
      // simplify
      return false;
    }
  }

  // Check if there are no other dependencies from funcName
  auto deps = FormulaUtil::getDependencies(formula, Expression::Type::FUNCTION,
                                           false, false);

  // Get the set of functions actually defined in this formula
  auto definedFuncs =
      FormulaUtil::getDefinitions(formula, Expression::Type::FUNCTION, false);
  std::set<std::string> definedFuncSet(definedFuncs.begin(),
                                       definedFuncs.end());

  for (const auto& dep : deps) {
    if (dep.first == funcName && dep.second != refFuncName) {
      return false;  // Has other dependencies
    }
  }

  for (const auto& dep : deps) {
    // Check if no other functions (besides funcName) depend on refFuncName
    // This prevents incorrect replacements like in A000472 where both a(n) and
    // c(n) depend on b(n), but replacing b with a would make c(n) incorrectly
    // depend on a(n)
    // Only consider dependencies on functions that are actually defined in this
    // formula Also exclude self-references (refFuncName depending on itself) as
    // these are valid recursive definitions
    if (dep.first != funcName && dep.first != refFuncName &&
        dep.second == refFuncName &&
        definedFuncSet.find(dep.first) != definedFuncSet.end()) {
      return false;  // Another function also depends on refFuncName
    }

    // Check if the referenced function (refFuncName) depends on other functions
    // defined in this formula. This prevents issues like in A001577 where
    // d(n) depends on both b(n) and c(n), making the simplification too
    // complex.
    if (dep.first == refFuncName &&
        definedFuncSet.find(dep.second) != definedFuncSet.end() &&
        dep.second != refFuncName) {  // Exclude self-references
      return false;  // refFuncName depends on other functions in this formula
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
  } else if (arg.type == Expression::Type::SUM && arg.children.size() == 2 &&
             arg.children[0].type == Expression::Type::PARAMETER &&
             arg.children[1].type == Expression::Type::CONSTANT) {
    // Already has an offset on the left side, adjust it
    arg.children[1].value -= offset;
    ExpressionUtil::normalize(expr.children[0]);
  }
  // For PARAMETER type, no adjustment needed for the left side
}

// Helper function to perform the replacement
void performReplacement(
    Formula& formula, const std::string& funcName,
    const std::string& refFuncName, const Number& offset,
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

void FormulaSimplify::replaceSimpleRecursiveRefs(Formula& formula) {
  // Find all functions
  auto funcs = FormulaUtil::getDefinitions(formula);

  // Track which functions were created by this algorithm
  std::set<std::string> processedRecursiveFuncs;

  // For each function, check if it's a simple reference to another recursive
  // function
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
                                    processedRecursiveFuncs, refFuncName,
                                    offset)) {
      continue;
    }

    // Collect all entries for the referenced function
    Formula refFuncs;
    formula.collectFunctionEntries(refFuncName, refFuncs);

    // Check if simplification would create negative initial term indices
    bool hasNegativeIndices = false;
    for (const auto& entry : refFuncs.entries) {
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
    performReplacement(formula, funcName, refFuncName, offset,
                       refFuncs.entries);

    // Mark this function as processed
    processedRecursiveFuncs.insert(funcName);
  }
}

void FormulaSimplify::replaceGeometricProgressions(Formula& formula) {
  // collect functions
  std::set<std::string> funcs;
  for (auto& e : formula.entries) {
    if (ExpressionUtil::isSimpleFunction(e.first)) {
      funcs.insert(e.first.name);
    }
  }
  // collect and check their ratios and initial values
  std::map<std::string, Number> ratios, initialValues;
  std::map<std::string, Expression> params;
  std::map<Number, Number> constants;
  for (auto& f : funcs) {
    constants.clear();
    bool found_ratio = false;
    Number ratio = 1;
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
        // Check if it's a product: c * f(n-1)
        if (val.type != Expression::Type::PRODUCT) {
          found_ratio = false;
          break;
        }
        if (val.children.size() != 2) {
          found_ratio = false;
          break;
        }
        // One child should be a constant (the ratio)
        // The other should be f(n-1)
        Expression predecessor(
            Expression::Type::SUM, "",
            {params[f],
             Expression(Expression::Type::CONSTANT, "", Number(-1))});
        Expression prevTerm(Expression::Type::FUNCTION, f, {predecessor});
        
        int constantIdx = -1;
        int functionIdx = -1;
        if (val.children.at(0).type == Expression::Type::CONSTANT) {
          constantIdx = 0;
          functionIdx = 1;
        } else if (val.children.at(1).type == Expression::Type::CONSTANT) {
          constantIdx = 1;
          functionIdx = 0;
        } else {
          found_ratio = false;
          break;
        }
        
        if (val.children.at(functionIdx) != prevTerm) {
          found_ratio = false;
          break;
        }
        ratio = val.children.at(constantIdx).value;
        found_ratio = true;
      } else {
        found_ratio = false;
        break;
      }
    }
    if (!found_ratio || constants.find(Number::ZERO) == constants.end()) {
      continue;
    }
    auto initial = constants.at(Number::ZERO);
    // Verify that all constant terms match the geometric progression: a * r^n
    for (const auto& c : constants) {
      auto expected_val = initial;
      for (Number i = Number::ZERO; i < c.first; i += Number::ONE) {
        expected_val *= ratio;
      }
      if (c.second != expected_val) {
        found_ratio = false;
        break;
      }
    }
    if (found_ratio) {
      ratios[f] = ratio;
      initialValues[f] = initial;
    }
  }
  // Replace geometric progressions with exponential formulas
  for (auto& f : funcs) {
    if (ratios.find(f) == ratios.end()) {
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
    // add simple function with exponential formula
    Expression power(
        Expression::Type::POWER, "",
        {Expression(Expression::Type::CONSTANT, "", ratios[f]), params[f]});
    Expression result;
    if (initialValues[f] == Number::ONE) {
      // f(n) = r^n
      result = power;
    } else {
      // f(n) = a * r^n
      result = Expression(
          Expression::Type::PRODUCT, "",
          {Expression(Expression::Type::CONSTANT, "", initialValues[f]), power});
    }
    ExpressionUtil::normalize(result);
    Expression func(Expression::Type::FUNCTION, f, {params[f]});
    formula.entries[func] = result;
  }
}