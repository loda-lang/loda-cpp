#pragma once

#include <memory>
#include <vector>

#include "number.hpp"
#include "program.hpp"

class Expression {
 public:
  enum class Type { CONSTANT, PARAMETER, PLUS, MINUS, MUL, DIV, RECURRENCE };

  Type type;
  std::string name;
  Number value;
  std::vector<std::shared_ptr<Expression>> children;

  std::string to_string() const;

  void addChild(Type type, const std::string& name = "",
                Number value = Number::ZERO);

 private:
  std::string to_string_op(const std::string& op) const;
};

class RecurrenceRelation {
 public:
  static std::pair<bool, RecurrenceRelation> fromProgram(const Program& p);

  std::string to_string() const;

  std::vector<std::pair<Expression, Expression>> entries;
};
