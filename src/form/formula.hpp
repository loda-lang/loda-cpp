#pragma once

#include <map>

#include "form/expression.hpp"
#include "lang/program.hpp"

class Formula {
 public:
  std::string toString() const;

  void clear();

  bool contains(const Expression& search) const;

  bool containsFunctionDef(const std::string& fname) const;

  std::multimap<std::string, std::string> getFunctionDeps(
      bool transitive, bool ignoreSelf) const;

  bool isRecursive(std::string funcName) const;

  void replaceAll(const Expression& from, const Expression& to);

  void replaceName(const std::string& from, const std::string& to);

  void substituteFunction(const std::string& from, const Expression& to);

  void collectEntries(const std::string& name, Formula& target);

  void collectEntries(const Expression& e, Formula& target);

  std::map<std::string, std::string> resolveIdentities();

  void resolveSimpleFunctions();

  void resolveSimpleRecursions();

  std::map<Expression, Expression> entries;
};
