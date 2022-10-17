#include "expression.hpp"

#include <algorithm>
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

Expression::Expression(Type type, const std::string& name,
                       std::initializer_list<Expression> children)
    : type(type), name(name) {
  for (auto& c : children) {
    newChild(c);
  }
}

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

bool Expression::operator==(const Expression& e) const {
  if (type != e.type) {
    return false;
  }
  // same type => compare content
  switch (type) {
    case Expression::Type::CONSTANT:
      return (value == e.value);
    case Expression::Type::PARAMETER:
      return (name == e.name);
    case Expression::Type::FUNCTION:
      return (name == e.name && equalChildren(e));
    case Expression::Type::NEGATION:
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
    case Expression::Type::MODULUS:
      return equalChildren(e);
  }
  throw std::runtime_error("internal error");  // unreachable
}

bool Expression::operator!=(const Expression& e) const { return !(*this == e); }

bool Expression::operator<(const Expression& e) const {
  if (type != e.type) {
    return (type < e.type);
  }
  // same type => compare content
  switch (type) {
    case Expression::Type::CONSTANT:
      return (value < e.value);
    case Expression::Type::PARAMETER:
      return (name < e.name);
    case Expression::Type::FUNCTION:
      if (name != e.name) {
        return (name < e.name);
      }
      return lessChildren(e);
    case Expression::Type::NEGATION:
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
    case Expression::Type::MODULUS:
      return lessChildren(e);
  }
  throw std::runtime_error("internal error");  // unreachable
}

bool Expression::equalChildren(const Expression& e) const {
  if (children.size() != e.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (*children[i] != *e.children[i]) {  // dereference!
      return false;
    }
  }
  return true;
}

bool Expression::lessChildren(const Expression& e) const {
  if (children.size() < e.children.size()) {
    return true;
  } else if (children.size() > e.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (*children[i] < *e.children[i]) {  // dereference!
      return true;
    } else if (*e.children[i] < *children[i]) {  // dereference!
      return false;
    }
  }
  return false;  // equal
}

Expression& Expression::newChild(const Expression& e) {
  children.emplace_back(new Expression(e));
  return *children.back();
}

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
  e.print(out, 0, true, Expression::Type::CONSTANT);
  return out;
}

std::string Expression::toString() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void Expression::print(std::ostream& out, size_t index, bool isRoot,
                       Expression::Type parentType) const {
  const bool brackets = needsBrackets(index, isRoot, parentType);
  if (brackets) {
    out << "(";
  }
  switch (type) {
    case Expression::Type::CONSTANT:
      out << value;
      break;
    case Expression::Type::PARAMETER:
      out << name;
      break;
    case Expression::Type::NEGATION:
      out << "-";
      children.front()->print(out, index, false, type);
      break;
    case Expression::Type::FUNCTION:
      out << name << "(";
      printChildren(out, ",", isRoot, parentType);
      out << ")";
      break;
    case Expression::Type::SUM:
      printChildren(out, "+", isRoot, parentType);
      break;
    case Expression::Type::DIFFERENCE:
      printChildren(out, "-", isRoot, parentType);
      break;
    case Expression::Type::PRODUCT:
      printChildren(out, "*", isRoot, parentType);
      break;
    case Expression::Type::FRACTION:
      printChildren(out, "/", isRoot, parentType);
      break;
    case Expression::Type::POWER:
      printChildren(out, "^", isRoot, parentType);
      break;
    case Expression::Type::MODULUS:
      printChildren(out, "%", isRoot, parentType);
      break;
  }
  if (brackets) {
    out << ")";
  }
}

bool Expression::needsBrackets(size_t index, bool isRoot,
                               Expression::Type parentType) const {
  if (isRoot) {
    return false;
  }
  if (type == Expression::Type::PARAMETER) {
    return false;
  }
  if (type == Expression::Type::CONSTANT && (Number(-1) < value)) {
    return false;
  }
  if (type == Expression::Type::FUNCTION ||
      parentType == Expression::Type::FUNCTION) {
    return false;
  }
  if (type == Expression::Type::NEGATION &&
      (parentType == Expression::Type::SUM ||
       parentType == Expression::Type::DIFFERENCE) &&
      index == 0) {
    return false;
  }
  if (type == Expression::Type::PRODUCT || type == Expression::Type::POWER ||
      type == Expression::Type::FRACTION || type == Expression::Type::MODULUS) {
    if (parentType == Expression::Type::SUM) {
      return false;
    }
    if (parentType == Expression::Type::DIFFERENCE &&
        (children.front()->type != Expression::Type::CONSTANT ||
         Number(-1) < children.front()->value)) {
      return false;
    }
  }
  if (type == Expression::Type::POWER &&
      parentType == Expression::Type::PRODUCT) {
    return false;
  }
  return true;
}

void Expression::printChildren(std::ostream& out, const std::string& op,
                               bool isRoot, Expression::Type parentType) const {
  for (size_t i = 0; i < children.size(); i++) {
    if (i > 0) {
      out << op;
    }
    children[i]->print(out, i, false, type);
  }
}
