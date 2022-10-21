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

void Formula::replaceAll(const Expression& from, const Expression& to) {
  auto newEntries = entries;
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
  auto newEntries = entries;
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
