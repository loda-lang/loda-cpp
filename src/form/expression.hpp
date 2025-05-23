#pragma once

#include <vector>

#include "math/number.hpp"

/**
 * Arithmitic expression representation. An expression is an n-ary tree
 * where every node has a type, an optional name and an optional value.
 * The name attribute is used for functions and variables. The value
 * attribute is used for constants.
 *
 * Example: max(3+b(n),7/4)
 */
class Expression {
 public:
  enum class Type {
    CONSTANT,
    PARAMETER,
    FUNCTION,
    VECTOR,
    LOCAL,
    SUM,
    PRODUCT,
    FRACTION,
    POWER,
    MODULUS,
    IF,
    FACTORIAL  // n! (factorial)
  };

  Expression();

  explicit Expression(Type type, const std::string& name = "",
                      const Number& value = Number::ZERO);

  Expression(Type type, const std::string& name,
             std::initializer_list<Expression> children);

  Expression(const Expression& e);

  Expression(Expression&& e);

  Expression& operator=(const Expression& e);

  Expression& operator=(Expression&& e);

  inline bool operator==(const Expression& e) const { return compare(e) == 0; }

  inline bool operator!=(const Expression& e) const { return compare(e) != 0; }

  inline bool operator<(const Expression& e) const { return compare(e) == -1; }

  inline bool operator>(const Expression& e) const { return compare(e) == 1; }

  int compare(const Expression& e) const;

  bool contains(const Expression& e) const;

  bool contains(Type t) const;

  bool contains(Type t, const std::string& name) const;

  size_t numTerms() const;

  Expression& newChild(const Expression& e);

  Expression& newChild(Type type, const std::string& name = "",
                       const Number& value = Number::ZERO);

  void replaceAll(const Expression& from, const Expression& to);

  void replaceInside(const Expression& from, const Expression& to,
                     const Expression::Type type);

  void replaceName(const std::string& from, const std::string& to);

  // Replace all sub-expressions of a given type, name, and arity with a new
  // type
  void replaceType(Type currentType, const std::string& name, size_t arity,
                   Type newType);

  friend std::ostream& operator<<(std::ostream& out, const Expression& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression> children;

 private:
  void assertNumChildren(size_t num) const;

  int compareChildren(const Expression& e) const;

  void print(std::ostream& out, bool isRoot, Expression::Type parentType) const;

  void printExtracted(std::ostream& out) const;

  void printChildren(std::ostream& out, const std::string& op) const;

  void printChildrenWrapped(std::ostream& out, const std::string& op,
                            const std::string& prefix,
                            const std::string& suffix) const;

  bool needsBrackets(bool isRoot, Expression::Type parentType) const;
};
