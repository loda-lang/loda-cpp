#include "form/formula.hpp"

#include <set>

#include "form/expression_util.hpp"

std::string Formula::toString(const std::string& sep, bool brackets) const {
  std::string result;
  bool first = true;
  for (auto it = entries.rbegin(); it != entries.rend(); it++) {
    if (!first) {
      result += sep;
    }
    if (brackets && entries.size() > 1) {
      result += "(";
    }
    result += it->first.toString() + " = " + it->second.toString();
    if (brackets && entries.size() > 1) {
      result += ")";
    }
    first = false;
  }
  return result;
}

void Formula::clear() { entries.clear(); }

bool Formula::contains(const Expression& search) const {
  return std::any_of(entries.begin(), entries.end(),
                     [&](const std::pair<Expression, Expression>& e) {
                       return e.first.contains(search) ||
                              e.second.contains(search);
                     });
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

void Formula::replaceInside(const Expression& from, const Expression& to,
                            const Expression::Type type) {
  std::map<Expression, Expression> newEntries;
  for (const auto& e : entries) {
    auto key = e.first;
    auto value = e.second;
    key.replaceInside(from, to, type);
    value.replaceInside(from, to, type);
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
  for (const auto& c : e.children) {
    collectEntries(c, target);
  }
}

void Formula::collectFunctionEntries(const std::string& func,
                                     Formula& target) const {
  for (const auto& entry : entries) {
    if (entry.first.type == Expression::Type::FUNCTION &&
        entry.first.name == func) {
      target.entries.insert(entry);
    }
  }
}
