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

bool FormulaUtil::hasMutualRecursion(const Formula& formula,
                                     Expression::Type type) {
  // Get transitive dependencies between functions
  auto deps = getDependencies(formula, type, true, false);

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
  for (const auto& funcA : funcNames) {
    for (const auto& funcB : funcNames) {
      if (funcA == funcB) continue;

      // Check if A depends on B
      bool aToB = false;
      auto rangeA = deps.equal_range(funcA);
      for (auto it = rangeA.first; it != rangeA.second; ++it) {
        if (it->second == funcB) {
          aToB = true;
          break;
        }
      }

      // Check if B depends on A
      bool bToA = false;
      auto rangeB = deps.equal_range(funcB);
      for (auto it = rangeB.first; it != rangeB.second; ++it) {
        if (it->second == funcA) {
          bToA = true;
          break;
        }
      }

      // If both A->B and B->A exist, we have mutual recursion
      if (aToB && bToA) {
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

Number FormulaUtil::getMinimumBaseCase(const Formula& formula,
                                       const std::string& funcName) {
  Number minBaseCase = Number::INF;
  for (const auto& entry : formula.entries) {
    auto left = entry.first;
    if (left.type == Expression::Type::FUNCTION && left.name == funcName &&
        left.children.size() == 1 &&
        left.children[0].type == Expression::Type::CONSTANT) {
      if (minBaseCase == Number::INF || left.children[0].value < minBaseCase) {
        minBaseCase = left.children[0].value;
      }
    }
  }
  return minBaseCase;
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
