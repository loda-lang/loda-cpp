#include "form/formula_util.hpp"

#include <set>

#include "form/expression_util.hpp"

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
