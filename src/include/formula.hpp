#pragma once

#include "expression.hpp"
#include "program.hpp"

class Formula {
 public:
  static std::pair<bool, Formula> fromProgram(const Program& p);

  std::string to_string() const;

 private:
  Expression* findEndtry(const Expression& left);

  std::vector<std::pair<Expression, Expression>> entries;
};
