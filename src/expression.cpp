#include "expression.hpp"

#include <sstream>

Expression::Expression()
    : type(Expression::Type::CONSTANT), value(Number::ZERO){};

Expression::~Expression() {
  for (auto c : children) {
    delete c;
  }
}

Expression::Expression(Type type, const std::string& name, const Number& value)
    : type(type), name(name), value(value){};

Expression::Expression(const Expression& e) { *this = e; }

Expression::Expression(Expression&& e) { *this = std::move(e); }

Expression& Expression::operator=(const Expression& e) {
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
      children[i] = new Expression(*e.children[i]);
    }
  }
  return *this;
}

Expression& Expression::operator=(Expression&& e) {
  if (this != &e) {
    for (auto c : children) {
      delete c;
    }
    type = e.type;
    name = std::move(e.name);
    value = std::move(e.value);
    children = std::move(e.children);
    e.children.clear();
  }
  return *this;
}

bool Expression::operator==(const Expression& e) {
  if (type != e.type) {
    return false;
  }
  switch (e.type) {
    case Expression::Type::CONSTANT:
      return (value == e.value);
    case Expression::Type::PARAMETER:
      return (name == e.name);
    case Expression::Type::FUNCTION:
      return (name == e.name && equalChildren(e));
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
      return equalChildren(e);
  }
}

bool Expression::operator!=(const Expression& e) { return !(*this == e); }

Expression& Expression::newChild(Expression::Type type, const std::string& name,
                                 const Number& value) {
  children.emplace_back(new Expression(type, name, value));
  return *children.back();
}

void Expression::replaceAll(const Expression& from, const Expression& to) {
  if (*this == from) {
    *this = to;
  } else {
    for (auto& c : children) {
      c->replaceAll(from, to);
    }
  }
}

std::ostream& operator<<(std::ostream& out, const Expression& e) {
  switch (e.type) {
    case Expression::Type::CONSTANT:
      out << e.value;
      break;
    case Expression::Type::PARAMETER:
      out << e.name;
      break;
    case Expression::Type::FUNCTION:
      out << e.name << "(" << *e.children.front() << ")";
      break;
    case Expression::Type::SUM:
      e.printChildren(out, "+");
      break;
    case Expression::Type::DIFFERENCE:
      e.printChildren(out, "-");
      break;
    case Expression::Type::PRODUCT:
      e.printChildren(out, "*");
      break;
    case Expression::Type::FRACTION:
      e.printChildren(out, "/");
      break;
  }
  return out;
}

std::string Expression::toString() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void Expression::printChildren(std::ostream& out, const std::string& op) const {
  for (size_t i = 0; i < children.size(); i++) {
    if (i > 0) {
      out << op;
    }
    out << *children[i];
  }
}

bool Expression::equalChildren(const Expression& e) {
  if (children.size() != e.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (*children[i] != *e.children[i]) {
      return false;
    }
  }
  return true;
}
