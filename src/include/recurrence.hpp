#pragma once

#include <memory>
#include <vector>

#include "number.hpp"
#include "program.hpp"

class Expression {
  enum class Type { CONSTANT, PARAMETER, PLUS, MINUS, MUL, DIV, RECURRENCE };

 public:
  Type type;
  Number value;
  std::unique_ptr<Expression> left, right;

  std::string to_string() const;
};

class RecurrenceRelation {
 public:
  static std::pair<bool, RecurrenceRelation> fromProgram(const Program& p);

  std::string to_string() const;

  std::vector<std::pair<Expression, Expression>> entries;
};
