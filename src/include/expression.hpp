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

  bool operator==(const Expression& e) const;

  bool operator!=(const Expression& e) const;

  bool operator<(const Expression& e) const;

  Expression& newChild(const Expression& e);

  Expression& newChild(Type type, const std::string& name = "",
                       const Number& value = Number::ZERO);

  void replaceAll(const Expression& from, const Expression& to);

  void normalize();

  friend std::ostream& operator<<(std::ostream& out, const Expression& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression*> children;

 private:
  void print(std::ostream& out, bool isRoot, Expression::Type parentType) const;

  void printChildren(std::ostream& out, const std::string& op, bool isRoot,
                     Expression::Type parentType) const;

  bool needsBrackets(Expression::Type parentType, bool isRoot) const;

  bool equalChildren(const Expression& e) const;

  bool lessChildren(const Expression& e) const;

  bool mergeTwoChildren();

  bool mergeAllChildren();

  bool pullUpChildren();

  bool simplifyNegativeProduct();
};
