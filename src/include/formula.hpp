#pragma once

#include <map>

#include "expression.hpp"
#include "program.hpp"

class Formula {
 public:
  static std::pair<bool, Formula> fromProgram(const Program& p, bool pariMode);

  std::string toString() const;

 private:
  std::map<Expression, Expression> entries;

  bool update(const Operation& op, bool pariMode);

  void replaceAll(const Expression& from, const Expression& to);
};
