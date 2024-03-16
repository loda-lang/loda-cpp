#pragma once

#include <vector>

#include "lang/number.hpp"

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
    IF
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

  void replaceName(const std::string& from, const std::string& to);

  friend std::ostream& operator<<(std::ostream& out, const Expression& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression> children;

 private:
  void assertNumChildren(size_t num) const;

  int compareChildren(const Expression& e) const;

  void print(std::ostream& out, size_t index, bool isRoot,
             Expression::Type parentType) const;

  void printExtracted(std::ostream& out, size_t index, bool isRoot,
                      Expression::Type parentType) const;

  void printChildren(std::ostream& out, const std::string& op, bool isRoot,
                     Expression::Type parentType) const;

  void printChildrenWrapped(std::ostream& out, const std::string& op,
                            bool isRoot, Expression::Type parentType,
                            const std::string& prefix,
                            const std::string& suffix) const;

  bool needsBrackets(size_t index, bool isRoot,
                     Expression::Type parentType) const;
};
