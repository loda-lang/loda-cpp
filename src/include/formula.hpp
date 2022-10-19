#pragma once

#include <map>

#include "expression.hpp"
#include "program.hpp"

class Formula {
 public:
  std::string toString(bool pariMode) const;

  void clear();

  bool contains(const Expression& search) const;

  void replaceAll(const Expression& from, const Expression& to);

  void replaceName(const std::string& from, const std::string& to);

  void collectEntries(const std::string& name, Formula& target);

  void collectEntries(const Expression& e, Formula& target);

  std::map<Expression, Expression> entries;
};
