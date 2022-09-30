#pragma once

#include <memory>
#include <vector>

#include "number.hpp"

class Expression {
 public:
  enum class Type { CONSTANT, PARAMETER, PLUS, MINUS, MUL, DIV, FUNCTION };

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

// ===============================================================================

class Expression2 {
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

  Expression2();

  ~Expression2();

  Expression2(Type type, const std::string& name = "",
              const Number& value = Number::ZERO);

  Expression2(const Expression2& e);

  Expression2(Expression2&& e);

  Expression2& operator=(const Expression2& e);

  Expression2& operator=(Expression2&& e);

  bool operator==(const Expression2& e);

  bool operator!=(const Expression2& e);

  Expression2& newChild(Type type, const std::string& name = "",
                        const Number& value = Number::ZERO);

  friend std::ostream& operator<<(std::ostream& out, const Expression2& e);

  std::string toString() const;

  Type type;
  std::string name;
  Number value;
  std::vector<Expression2*> children;

 private:
  void printChildren(std::ostream& out, const std::string& op) const;

  bool equalChildren(const Expression2& e);
};
