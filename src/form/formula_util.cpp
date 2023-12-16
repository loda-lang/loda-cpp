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
  for (const auto& c : target.children) {
    replaceFunction(*c, func, param, val);
  }
  ExpressionUtil::normalize(target);
  if (target.type != Expression::Type::FUNCTION ||
      target.children.size() != 1 || target.name != func) {
    return;
  }
  auto updated = val;
  updated.replaceAll(param, *target.children.front());
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
      params[e.first.name] = *e.first.children.front();
      defs[e.first.name] = e.second;
    }
  }
  // filter out non-simple functions
  auto deps = formula.getFunctionDeps(false, false);
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
      if (it.first == f && formula.containsFunctionDef(it.second)) {
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
      auto arg_type = e.first.children.front()->type;
      if (arg_type == Expression::Type::CONSTANT) {
        if (e.second.type != Expression::Type::CONSTANT) {
          constants.clear();
          break;
        }
        constants[e.first.children.front()->value] = e.second.value;
      } else if (arg_type == Expression::Type::PARAMETER) {
        params[f] = *e.first.children.front();
        auto val = e.second;
        if (val.type != Expression::Type::SUM) {
          found_slope = false;
          break;
        }
        if (val.children.size() != 2) {
          found_slope = false;
          break;
        }
        if (val.children.at(1)->type != Expression::Type::CONSTANT) {
          found_slope = false;
          break;
        }
        Expression predecessor(
            Expression::Type::SUM, "",
            {params[f],
             Expression(Expression::Type::CONSTANT, "", Number(-1))});
        Expression prevTerm(Expression::Type::FUNCTION, f, {predecessor});
        if (*val.children.at(0) != prevTerm) {
          found_slope = false;
          break;
        }
        slope = val.children.at(1)->value;
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

int64_t getRecursionDepthInExpr(const Expression& expr) {
  int64_t depth = 0;
  if (expr.type == Expression::Type::FUNCTION && expr.children.size() == 1) {
    const auto& arg = *expr.children[0];
    if (arg.type == Expression::Type::SUM && arg.children.size() == 2 &&
        arg.children[0]->type == Expression::Type::PARAMETER &&
        arg.children[1]->type == Expression::Type::CONSTANT) {
      depth = -arg.children[1]->value.asInt();
    }
  }
  for (auto c : expr.children) {
    depth = std::max<int64_t>(depth, getRecursionDepthInExpr(*c));
  }
  return depth;
}

int64_t FormulaUtil::getRecursionDepth(const Formula& formula,
                                       const std::string& fname) {
  for (auto& e : formula.entries) {
    const auto& left = e.first;
    if (left.type == Expression::Type::FUNCTION && left.name == fname &&
        left.children.size() == 1 &&
        left.children[0]->type == Expression::Type::PARAMETER) {
      return getRecursionDepthInExpr(e.second);
    }
  }
  return -1;
}
