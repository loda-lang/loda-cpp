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

int Expression::compare(const Expression& e) const {
  if (type < e.type) {
    return -1;
  } else if (e.type < type) {
    return 1;
  }
  // same type => compare content
  switch (type) {
    case Expression::Type::CONSTANT:
      if (value < e.value) {
        return -1;
      } else if (e.value < value) {
        return 1;
      } else {
        return 0;
      }
    case Expression::Type::PARAMETER:
      if (name < e.name) {
        return -1;
      } else if (e.name < name) {
        return 1;
      } else {
        return 0;
      }
    case Expression::Type::FUNCTION:
      if (name < e.name) {
        return -1;
      } else if (e.name < name) {
        return 1;
      } else {
        return compareChildren(e);
      }
    case Expression::Type::NEGATION:
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
    case Expression::Type::MODULUS:
    case Expression::Type::IF:
      return compareChildren(e);
  }
  return 0;  // equal
}

bool Expression::contains(const Expression& e) const {
  if (*this == e) {
    return true;
  }
  for (auto c : children) {
    if (c->contains(e)) {
      return true;
    }
  }
  return false;
}

size_t Expression::numTerms() const {
  size_t result = 1;
  for (auto c : children) {
    result += c->numTerms();
  }
  return result;
}

void Expression::assertNumChildren(size_t num) const {
  if (children.size() != num) {
    throw std::runtime_error("unexpected number of children: " + toString());
  }
}

int Expression::compareChildren(const Expression& e) const {
  if (children.size() < e.children.size()) {
    return -1;
  } else if (children.size() > e.children.size()) {
    return 1;
  }
  // same number of children => compare them one by one
  for (size_t i = 0; i < children.size(); i++) {
    auto r = children[i]->compare(*e.children[i]);
    if (r != 0) {
      return r;
    }
  }
  return 0;  // equal
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

void Expression::replaceName(const std::string& from, const std::string& to) {
  if (name == from) {
    name = to;
  }
  for (auto& c : children) {
    c->replaceName(from, to);
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
      assertNumChildren(1);
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
    case Expression::Type::IF:
      assertNumChildren(3);
      out << name << "if(n==";
      printChildren(out, ",", isRoot, parentType);
      out << ")";
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
  if (type == Expression::Type::IF || parentType == Expression::Type::IF) {
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
