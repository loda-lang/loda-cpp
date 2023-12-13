#include "form/formula_alt.hpp"

#include "form/expression_util.hpp"
#include "sys/log.hpp"

bool altExists(const Alternatives& alt, const AltEntry& entry) {
  auto range = alt.equal_range(entry.first);
  for (auto it = range.first; it != range.second; it++) {
    if (it->second == entry.second) {
      return true;
    }
  }
  return false;
}

bool resolve(const AltEntry& entry, const Expression& left, Expression& right) {
  if (right.type == Expression::Type::FUNCTION) {
    auto lookup = ExpressionUtil::newFunction(right.name);
    if (lookup != left && lookup == entry.first) {
      auto replacement = entry.second;  // copy
      replacement.replaceAll(ExpressionUtil::newParameter(),
                             *right.children[0]);
      ExpressionUtil::normalize(replacement);
      right = replacement;
      return true;  // must stop here
    }
  }
  bool resolved = false;
  for (auto c : right.children) {
    if (resolve(entry, left, *c)) {
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
    for (auto& lookup : alt) {
      std::set<std::string> names_before, names_after;
      auto right = e.second;  // copy
      ExpressionUtil::collectNames(right, Expression::Type::FUNCTION, names_before);
      if (resolve(lookup, e.first, right)) {
        ExpressionUtil::collectNames(right, Expression::Type::FUNCTION, names_after);
        std::pair<Expression, Expression> p(e.first, right);
        if (names_after != names_before && !altExists(newAlt, p)) {
          Log::get().debug("Found alternative " + p.first.toString() + " = " +
                           p.second.toString());
          newAlt.insert(p);
          found = true;
        }
      }
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
  alt.insert(formula.entries.begin(), formula.entries.end());
  bool updated = false;
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
