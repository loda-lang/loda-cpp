#include "expression.hpp"

#include <sstream>

std::string Expression::to_string() const {
  switch (type) {
    case Expression::Type::CONSTANT:
      return value.to_string();
    case Expression::Type::PARAMETER:
      return name;
    case Expression::Type::PLUS:
      return to_string_op("+");
    case Expression::Type::MINUS:
      return to_string_op("-");
    case Expression::Type::MUL:
      return to_string_op("*");
    case Expression::Type::DIV:
      return to_string_op("/");
    case Expression::Type::FUNCTION:
      return name + "(" + children.at(0)->to_string() + ")";
  }
}

std::string Expression::to_string_op(const std::string& op) const {
  std::string out;
  for (size_t i = 0; i < children.size(); i++) {
    if (i > 0) {
      out += op;
    }
    out += children[i]->to_string();
  }
  return out;
}

void Expression::addChild(Expression::Type type, const std::string& name,
                          Number value) {
  children.emplace_back(std::unique_ptr<Expression>(new Expression()));
  children.back()->type = type;
  children.back()->name = name;
  children.back()->value = value;
}

// ===============================================================================

Expression2::Expression2()
    : type(Expression2::Type::CONSTANT), value(Number::ZERO){};

Expression2::~Expression2() {
  for (auto c : children) {
    delete c;
  }
}

Expression2::Expression2(Type type, const std::string& name,
                         const Number& value)
    : type(type), name(name), value(value){};

Expression2::Expression2(const Expression2& e) { *this = e; }

Expression2::Expression2(Expression2&& e) { *this = std::move(e); }

Expression2& Expression2::operator=(const Expression2& e) {
  if (this != &e) {
    for (auto c : children) {
      delete c;
    }
    type = e.type;
    name = e.name;
    value = e.value;
    const size_t s = e.children.size();
    children.resize(s);
    for (size_t i = 0; i < s; i++) {
      children[i] = new Expression2(*e.children[i]);
    }
  }
  return *this;
}

Expression2& Expression2::operator=(Expression2&& e) {
  if (this != &e) {
    for (auto c : children) {
      delete c;
    }
    type = e.type;
    name = std::move(e.name);
    value = std::move(e.value);
    children = std::move(e.children);
  }
  return *this;
}

bool Expression2::operator==(const Expression2& e) {
  if (type != e.type) {
    return false;
  }
  switch (e.type) {
    case Expression2::Type::CONSTANT:
      return (value == e.value);
    case Expression2::Type::PARAMETER:
      return (name == e.name);
    case Expression2::Type::FUNCTION:
      return (name == e.name && equalChildren(e));
    case Expression2::Type::SUM:
    case Expression2::Type::DIFFERENCE:
    case Expression2::Type::PRODUCT:
    case Expression2::Type::FRACTION:
      return equalChildren(e);
  }
}

bool Expression2::operator!=(const Expression2& e) { return !(*this == e); }

Expression2& Expression2::newChild(Expression2::Type type,
                                   const std::string& name,
                                   const Number& value) {
  children.emplace_back(new Expression2(type, name, value));
  return *children.back();
}

std::ostream& operator<<(std::ostream& out, const Expression2& e) {
  switch (e.type) {
    case Expression2::Type::CONSTANT:
      out << e.value;
      break;
    case Expression2::Type::PARAMETER:
      out << e.name;
      break;
    case Expression2::Type::FUNCTION:
      out << e.name << "(" << *e.children.front() << ")";
      break;
    case Expression2::Type::SUM:
      e.printChildren(out, "+");
      break;
    case Expression2::Type::DIFFERENCE:
      e.printChildren(out, "-");
      break;
    case Expression2::Type::PRODUCT:
      e.printChildren(out, "*");
      break;
    case Expression2::Type::FRACTION:
      e.printChildren(out, "/");
      break;
  }
  return out;
}

std::string Expression2::toString() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void Expression2::printChildren(std::ostream& out,
                                const std::string& op) const {
  for (size_t i = 0; i < children.size(); i++) {
    if (i > 0) {
      out << op;
    }
    out << children[i];
  }
}

bool Expression2::equalChildren(const Expression2& e) {
  if (children.size() != e.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (children[i] != e.children[i]) {
      return false;
    }
  }
  return true;
}
