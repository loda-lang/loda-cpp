#pragma once

#include <vector>

#include "number.hpp"

class Expression {
 public:
  enum class Type {
    CONSTANT,
    PARAMETER,
    FUNCTION,
    SUM,
    DIFFERENCE,
    PRODUCT,
    FRACTION
  };

  Expression();

  ~Expression();

  Expression(Type type, const std::string& name = "",
             const Number& value = Number::ZERO);

  Expression(const Expression& e);

  Expression(Expression&& e);

  Expression& operator=(const Expression& e);

  Expression& operator=(Expression&& e);

  bool operator==(const Expression& e);

  bool operator!=(const Expression& e);

  Expression& newChild(const Expression& e);

  Expression& newChild(Type type, const std::string& name = "",
                       const Number& value = Number::ZERO);

  void replaceAll(const Expression& from, const Expression& to);

  friend std::ostream& operator<<(std::ostream& out, const Expression& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression*> children;

 private:
  void printChildren(std::ostream& out, const std::string& op) const;

  bool equalChildren(const Expression& e);
};
