#pragma once

#include <map>

#include "expression.hpp"
#include "program.hpp"

class Formula {
 public:
  std::string toString() const;

  void clear();

  bool contains(const Expression& search) const;

  bool containsFunctionDef(const std::string& fname) const;

  std::multimap<std::string, std::string> getFunctionDeps(
      bool transitive) const;

  bool isRecursive(std::string funcName) const;

  void replaceAll(const Expression& from, const Expression& to);

  void replaceName(const std::string& from, const std::string& to);

  void collectEntries(const std::string& name, Formula& target);

  void collectEntries(const Expression& e, Formula& target);

  void resolveIdentities();

  std::map<Expression, Expression> entries;
};
