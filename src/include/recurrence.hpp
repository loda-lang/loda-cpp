#pragma once

#include "expression.hpp"
#include "program.hpp"

class RecurrenceRelation {
 public:
  static std::pair<bool, RecurrenceRelation> fromProgram(const Program& p);

  std::string to_string() const;

 private:
  Expression* findEndtry(const Expression& left);

  std::vector<std::pair<Expression, Expression>> entries;
};
