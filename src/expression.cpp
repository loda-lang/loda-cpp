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
    case Expression::Type::NEGATION:
    case Expression::Type::SUM:
    case Expression::Type::DIFFERENCE:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
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
      return lessChildren(e);
  }
  throw std::runtime_error("internal error");  // unreachable
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
    case Expression::Type::NEGATION:
    case Expression::Type::POWER:
      break;
    case Expression::Type::SUM:
      if (children.size() > 1) {  // at least two elements
        std::sort(children.rbegin(), children.rend(), compareExprPtr);
        mergeAllChildren();
      }
      break;
    case Expression::Type::PRODUCT:
      if (children.size() > 1) {  // at least two elements
        std::sort(children.begin(), children.end(), compareExprPtr);
        mergeAllChildren();
      }
      break;
    case Expression::Type::DIFFERENCE:
    case Expression::Type::FRACTION:
      if (children.size() > 2) {  // at least three elements
        std::sort(children.rbegin(), children.rend() - 1, compareExprPtr);
        mergeAllChildren();
      }
      break;
  }
  if (pullUpChildren()) {
    normalize();
  }
  diffToNeg();
  removeNeutral();
  simplifyNegativeProduct();
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
  if (type == Expression::Type::FUNCTION) {
    out << name;
  }
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
      printChildren(out, ",", isRoot, parentType);
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
  }
  if (brackets) {
    out << ")";
  }
}

bool Expression::needsBrackets(size_t index, bool isRoot,
                               Expression::Type parentType) const {
  if (type == Expression::Type::PARAMETER) {
    return false;
  }
  if (type == Expression::Type::CONSTANT && (Number(-1) < value)) {
    return false;
  }
  if (isRoot && type != Expression::Type::FUNCTION) {
    return false;
  }
  if (type == Expression::Type::NEGATION &&
      (parentType == Expression::Type::SUM ||
       parentType == Expression::Type::DIFFERENCE) &&
      index == 0) {
    return false;
  }
  if (type == Expression::Type::PRODUCT || type == Expression::Type::POWER) {
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

bool Expression::mergeTwoChildren() {
  for (size_t i = 0; i + 1 < children.size(); i++) {
    auto c = children[i];
    auto d = children[i + 1];
    bool merged = false;
    if (c->type == Expression::Type::CONSTANT &&
        d->type == Expression::Type::CONSTANT) {
      switch (type) {
        case Expression::Type::SUM: {
          c->value += d->value;
          merged = true;
          break;
        }
        case Expression::Type::DIFFERENCE: {
          if (i == 0) {
            c->value -= d->value;
          } else {
            c->value += d->value;
          }
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
    } else if (*c == *d) {
      switch (type) {
        case Expression::Type::SUM: {
          *c = Expression(Expression::Type::PRODUCT);
          c->newChild(Expression::Type::CONSTANT, "", Number(2));
          c->newChild(*d);
          merged = true;
          break;
        }
        case Expression::Type::PRODUCT: {
          *c = Expression(Expression::Type::POWER);
          c->newChild(*d);
          c->newChild(Expression::Type::CONSTANT, "", Number(2));
          merged = true;
          break;
        }
        default:
          break;
      }
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

bool Expression::pullUpChildren() {
  if (type != Expression::Type::SUM && type != Expression::Type::DIFFERENCE &&
      type != Expression::Type::PRODUCT) {
    return false;
  }
  std::vector<Expression*> collected;
  auto it = children.begin();
  while (it != children.end()) {
    if ((*it)->type == type) {
      collected.insert(collected.end(), (*it)->children.begin(),
                       (*it)->children.end());
      it = children.erase(it);
    } else {
      it++;
    }
    // for differences, we can only pull up the first child
    if (type == Expression::Type::DIFFERENCE) {
      break;
    }
  }
  children.insert(children.begin(), collected.begin(), collected.end());
  return !collected.empty();
}

bool Expression::simplifyNegativeProduct() {
  if (type != Expression::Type::PRODUCT) {
    return false;
  }
  if (children.size() != 2) {
    return false;
  }
  if (children[0]->type != Expression::Type::CONSTANT) {
    return false;
  }
  if (children[0]->value != Number(-1)) {
    return false;
  }
  if (children[1]->type != Expression::Type::DIFFERENCE) {
    return false;
  }
  if (children[1]->children.size() != 2) {
    return false;
  }
  auto tmp = *children[1];
  std::swap(tmp.children[0], tmp.children[1]);
  *this = tmp;
  return true;
}

bool Expression::removeNeutral() {
  Number neutral;
  size_t start = 0;
  switch (type) {
    case Expression::Type::SUM:
      neutral = Number::ZERO;
      start = 0;
      break;
    case Expression::Type::DIFFERENCE:
      neutral = Number::ZERO;
      start = 1;
      break;
    case Expression::Type::PRODUCT:
      neutral = Number::ONE;
      start = 0;
      break;
    case Expression::Type::FRACTION:
      neutral = Number::ONE;
      start = 1;
      break;
    default:
      return false;
  }
  auto it = children.begin() + start;
  bool changed = false;
  while (it != children.end()) {
    if ((*it)->type == Expression::Type::CONSTANT && (*it)->value == neutral) {
      delete *it;
      it = children.erase(it);
      changed = true;
    } else {
      it++;
    }
  }
  return changed;
}

bool Expression::diffToNeg() {
  if (type == Expression::Type::DIFFERENCE && children.size() == 2 &&
      *children.front() ==
          Expression(Expression::Type::CONSTANT, "", Number::ZERO)) {
    auto c = *children[1];
    if (c.type == Expression::Type::NEGATION) {
      *this = *c.children[0];
    } else {
      *this = Expression(Expression::Type::NEGATION);
      newChild(c);
    }
    return true;
  }
  return false;
}
