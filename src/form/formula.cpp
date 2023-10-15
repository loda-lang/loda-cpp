#include "form/formula.hpp"

#include <iostream>
#include <set>

#include "form/expression_util.hpp"

std::string Formula::toString() const {
  std::string result;
  bool first = true;
  for (auto it = entries.rbegin(); it != entries.rend(); it++) {
    if (!first) {
      result += ", ";
    }
    result += it->first.toString() + " = " + it->second.toString();
    first = false;
  }
  return result;
}

void Formula::clear() { entries.clear(); }

bool Formula::contains(const Expression& search) const {
  for (auto& e : entries) {
    if (e.first.contains(search) || e.second.contains(search)) {
      return true;
    }
  }
  return false;
}

bool Formula::containsFunctionDef(const std::string& fname) const {
  for (const auto& e : entries) {
    if (e.first.type == Expression::Type::FUNCTION && e.first.name == fname) {
      return true;
    }
  }
  return false;
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
                 std::multimap<std::string, std::string>& deps) {
  if (e.type == Expression::Type::FUNCTION && !e.name.empty() &&
      !containsPair(deps, fname, e.name)) {
    deps.insert({fname, e.name});
  }
  for (auto c : e.children) {
    collectDeps(fname, *c, deps);
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

std::multimap<std::string, std::string> Formula::getFunctionDeps(
    bool transitive, bool ignoreSelf) const {
  std::multimap<std::string, std::string> deps;
  for (auto& e : entries) {
    if (e.first.type == Expression::Type::FUNCTION && !e.first.name.empty()) {
      collectDeps(e.first.name, e.second, deps);
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

bool Formula::isRecursive(std::string funcName) const {
  auto deps = getFunctionDeps(false, false);
  for (auto it : deps) {
    if (it.first == funcName && it.second == funcName) {
      return true;
    }
  }
  return false;
}

void Formula::replaceAll(const Expression& from, const Expression& to) {
  std::map<Expression, Expression> newEntries;
  for (const auto& e : entries) {
    auto key = e.first;
    auto value = e.second;
    key.replaceAll(from, to);
    value.replaceAll(from, to);
    newEntries[key] = value;
  }
  entries = newEntries;
}

void Formula::replaceName(const std::string& from, const std::string& to) {
  std::map<Expression, Expression> newEntries;
  for (const auto& e : entries) {
    auto key = e.first;
    auto value = e.second;
    key.replaceName(from, to);
    value.replaceName(from, to);
    newEntries[key] = value;
  }
  entries = newEntries;
}

void Formula::substituteFunction(const std::string& from,
                                 const Expression& to) {
  const auto param = ExpressionUtil::newParameter();
  for (auto& e : entries) {
    std::cout << "subst " << from << " -> " << to.toString() << " in "
              << e.second.toString() << std::endl;
    e.second.substituteFunction(from, to, param.name);
    ExpressionUtil::normalize(e.second);
  }
}

void Formula::collectEntries(const std::string& name, Formula& target) {
  for (auto& e : entries) {
    if (e.first.name == name) {
      auto it = target.entries.find(e.first);
      if (it == target.entries.end()) {
        target.entries.insert(e);
        collectEntries(e.second, target);
      }
    }
  }
}

void Formula::collectEntries(const Expression& e, Formula& target) {
  if (e.type == Expression::Type::FUNCTION && !e.name.empty()) {
    collectEntries(e.name, target);
  }
  for (auto c : e.children) {
    collectEntries(*c, target);
  }
}

void Formula::resolveIdentities(const std::string& main) {
  auto copy = entries;
  for (auto& e : copy) {
    if (!ExpressionUtil::isSimpleFunction(e.first, true) ||
        !ExpressionUtil::isSimpleFunction(e.second, false) ||
        e.first.name == main) {
      continue;
    }
    auto r = ExpressionUtil::newFunction(e.second.name);
    if (copy.find(r) != copy.end()) {
      std::cout << "candidate " << e.first.toString() << " = "
                << e.second.toString() << std::endl;
      entries.erase(e.first);

      substituteFunction(e.first.name, e.second);
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

void Formula::resolveSimpleFunctions() {
  // collect function definitions
  std::set<std::string> simple_funcs;
  std::map<std::string, Expression> params, defs;
  for (auto& e : entries) {
    if (ExpressionUtil::isSimpleFunction(e.first)) {
      simple_funcs.insert(e.first.name);
      params[e.first.name] = *e.first.children.front();
      defs[e.first.name] = e.second;
    }
  }
  // filter out non-simple functions
  auto deps = getFunctionDeps(false, false);
  for (auto& e : entries) {
    if (e.first.type != Expression::Type::FUNCTION) {
      continue;  // should not happen
    }
    auto f = e.first.name;
    bool is_simple = true;
    if (!ExpressionUtil::isSimpleFunction(e.first)) {
      is_simple = false;
    }
    for (auto it : deps) {
      if (it.first == f && containsFunctionDef(it.second)) {
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
    for (auto& e : entries) {
      replaceFunction(e.second, f, params[f], defs[f]);
    }
  }
}

void Formula::resolveSimpleRecursions() {
  // collect functions
  std::set<std::string> funcs;
  for (auto& e : entries) {
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
    for (auto& e : entries) {
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
    auto it = entries.begin();
    while (it != entries.end()) {
      if (it->first.name == f) {
        it = entries.erase(it);
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
    entries[func] = sum;
  }
}
