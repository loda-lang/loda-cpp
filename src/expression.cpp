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
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
      return equalChildren(e);
  }
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
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
      return lessChildren(e);
  }
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

bool compareExprPtr(const Expression* lhs, const Expression* rhs) {
  return (*lhs < *rhs);
}

void Expression::normalize() {
  for (auto& c : children) {
    c->normalize();
  }
  switch (type) {
    case Expression::Type::CONSTANT:
    case Expression::Type::PARAMETER:
    case Expression::Type::FUNCTION:
      break;
    case Expression::Type::SUM:
    case Expression::Type::PRODUCT:
      if (children.size() > 1) {  // at least two elements
        std::sort(children.begin(), children.end(), compareExprPtr);
        mergeAllChildren();
      }
      break;
    case Expression::Type::DIFFERENCE:
    case Expression::Type::FRACTION:
      if (children.size() > 2) {  // at least three elements
        std::sort(children.begin() + 1, children.end(), compareExprPtr);
        mergeAllChildren();
      }
      break;
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

bool Expression::equalChildren(const Expression& e) const {
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

bool Expression::lessChildren(const Expression& e) const {
  if (children.size() < e.children.size()) {
    return true;
  } else if (children.size() > e.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (children[i] < e.children[i]) {
      return true;
    } else if (e.children[i] < children[i]) {
      return false;
    }
  }
  return false;  // equal
}

bool Expression::mergeTwoChildren() {
  for (size_t i = 0; i + 1 < children.size(); i++) {
    auto c = children[i];
    auto d = children[i + 1];
    if (c->type != Expression::Type::CONSTANT ||
        d->type != Expression::Type::CONSTANT) {
      continue;
    }
    bool merged = false;
    switch (type) {
      case Expression::Type::SUM: {
        c->value += d->value;
        merged = true;
        break;
      }
      case Expression::Type::DIFFERENCE: {
        c->value -= d->value;
        merged = true;
        break;
      }
      case Expression::Type::PRODUCT: {
        c->value *= d->value;
        merged = true;
        break;
      }
      default:
        break;
    }
    if (merged) {
      delete d;
      children.erase(children.begin() + i + 1);
      return true;
    }
  }
  return false;
}

bool Expression::mergeAllChildren() {
  bool changed = false;
  while (mergeTwoChildren()) {
    changed = true;
  }
  return changed;
}
