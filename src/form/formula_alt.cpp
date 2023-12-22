#include "form/formula_alt.hpp"

#include "form/expression_util.hpp"
#include "sys/log.hpp"

bool resolve(const Alternatives& alt, const Expression& left,
             Expression& right) {
  if (right.type == Expression::Type::FUNCTION) {
    auto lookup = ExpressionUtil::newFunction(right.name);
    if (lookup != left) {
      auto range = alt.equal_range(lookup);
      for (auto it = range.first; it != range.second; it++) {
        auto replacement = it->second;
        replacement.replaceAll(ExpressionUtil::newParameter(),
                               right.children.front());
        ExpressionUtil::normalize(replacement);
        auto range2 = alt.equal_range(left);
        bool exists = false;
        for (auto it2 = range2.first; it2 != range2.second; it2++) {
          if (it2->second == replacement) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          right = replacement;
          return true;  // must stop here
        }
      }
    }
  }
  bool resolved = false;
  for (auto& c : right.children) {  // must be by reference!
    if (resolve(alt, left, c)) {
      resolved = true;
    }
  }
  ExpressionUtil::normalize(right);
  return resolved;
}

bool findAlternativesByResolve(Alternatives& alt) {
  auto newAlt = alt;  // copy
  bool found = false;
  for (auto& e : alt) {
    auto right = e.second;  // copy
    if (resolve(newAlt, e.first, right)) {
      std::pair<Expression, Expression> p(e.first, right);
      Log::get().debug("Found alternative " + p.first.toString() + " = " +
                       p.second.toString());
      newAlt.insert(p);
      found = true;
    }
  }
  if (found) {
    alt = newAlt;
  }
  return found;
}

bool applyAlternatives(const Alternatives& alt, Formula& f) {
  bool applied = false;
  for (auto& e : f.entries) {
    auto range = alt.equal_range(e.first);
    for (auto it = range.first; it != range.second; it++) {
      if (it->second == e.second) {
        continue;
      }
      Formula g = f;  // copy
      g.entries[e.first] = it->second;
      auto depsOld = f.getFunctionDeps(true, true);
      auto depsNew = g.getFunctionDeps(true, true);
      std::string debugMsg =
          " alternative " + e.first.toString() + " = " + it->second.toString();
      if (depsNew.size() < depsOld.size()) {
        e.second = it->second;
        applied = true;
        Log::get().debug("Applied" + debugMsg);
      } else {
        Log::get().debug("Skipped" + debugMsg);
      }
    }
  }
  return applied;
}

bool simplifyFormulaUsingAlternatives(Formula& formula) {
  // find and choose alternative function definitions
  Alternatives alt;
  bool updated = false;
  alt.insert(formula.entries.begin(), formula.entries.end());
  while (true) {
    if (!findAlternativesByResolve(alt)) {
      break;
    }
    if (!applyAlternatives(alt, formula)) {
      break;
    }
    updated = true;
    Log::get().debug("Updated formula: " + formula.toString());
  }
  return updated;
}
