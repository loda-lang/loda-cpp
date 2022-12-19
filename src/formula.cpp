#include "formula.hpp"

std::string Formula::toString(bool pariMode) const {
  std::string result;
  bool first = true;
  for (auto it = entries.rbegin(); it != entries.rend(); it++) {
    if (!first) {
      if (pariMode) {
        result += "; ";
      } else {
        result += ", ";
      }
    }
    if (pariMode && entries.size() > 1) {
      result += "(";
    }
    result += it->first.toString() + " = " + it->second.toString();
    if (pariMode && entries.size() > 1) {
      result += ")";
    }
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
  for (auto& e : entries) {
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
    bool transitive) const {
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
  return deps;
}

bool Formula::isRecursive(std::string funcName) const {
  auto deps = getFunctionDeps(false);
  for (auto it : deps) {
    if (it.first == funcName && it.second == funcName) {
      return true;
    }
  }
  return false;
}

void Formula::replaceAll(const Expression& from, const Expression& to) {
  std::map<Expression, Expression> newEntries;
  for (auto& e : entries) {
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
  for (auto& e : entries) {
    auto key = e.first;
    auto value = e.second;
    key.replaceName(from, to);
    value.replaceName(from, to);
    newEntries[key] = value;
  }
  entries = newEntries;
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
