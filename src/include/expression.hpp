#pragma once

#include <vector>

#include "number.hpp"

class Expression {
 public:
  enum class Type {
    CONSTANT,
    PARAMETER,
    FUNCTION,
    NEGATION,
    SUM,
    DIFFERENCE,
    PRODUCT,
    FRACTION,
    POWER,
    MODULUS
  };

  Expression();

  ~Expression();

  Expression(Type type, const std::string& name = "",
             const Number& value = Number::ZERO);

  Expression(Type type, const std::string& name,
             std::initializer_list<Expression> children);

  Expression(const Expression& e);

  Expression(Expression&& e);

  Expression& operator=(const Expression& e);

  Expression& operator=(Expression&& e);

  bool operator==(const Expression& e) const;

  bool operator!=(const Expression& e) const;

  bool operator<(const Expression& e) const;

  bool contains(const Expression& e);

  Expression& newChild(const Expression& e);

  Expression& newChild(Type type, const std::string& name = "",
                       const Number& value = Number::ZERO);

  void replaceAll(const Expression& from, const Expression& to);

  void replaceName(const std::string& from, const std::string& to);

  friend std::ostream& operator<<(std::ostream& out, const Expression& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression*> children;

 private:
  bool equalChildren(const Expression& e) const;

  bool lessChildren(const Expression& e) const;

  void print(std::ostream& out, size_t index, bool isRoot,
             Expression::Type parentType) const;

  void printChildren(std::ostream& out, const std::string& op, bool isRoot,
                     Expression::Type parentType) const;

  bool needsBrackets(size_t index, bool isRoot,
                     Expression::Type parentType) const;
};
