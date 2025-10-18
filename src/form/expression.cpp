#include "form/expression.hpp"

#include <algorithm>
#include <sstream>

Expression::Expression()
    : type(Expression::Type::CONSTANT), value(Number::ZERO) {};

Expression::Expression(Type type, const std::string& name, const Number& value)
    : type(type), name(name), value(value) {};

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
    type = e.type;
    name = e.name;
    value = e.value;
    children = e.children;
  }
  return *this;
}

Expression& Expression::operator=(Expression&& e) {
  if (this != &e) {
    type = e.type;
    name = std::move(e.name);
    value = std::move(e.value);
    children = std::move(e.children);
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
    case Expression::Type::VECTOR:
      // custom sorting for function/vector names
      if (name == e.name) {
        return compareChildren(e);
      } else if (name.empty()) {
        return 1;
      } else if (e.name.empty()) {
        return -1;
      } else if (std::islower(name[0]) && std::isupper(e.name[0])) {
        return 1;
      } else if (std::isupper(name[0]) && std::islower(e.name[0])) {
        return -1;
      } else if (name < e.name) {
        return 1;
      } else {
        return -1;
      }
    case Expression::Type::LOCAL:
      if (name < e.name) {
        return -1;
      } else if (e.name < name) {
        return 1;
      } else {
        return compareChildren(e);
      }
    case Expression::Type::SUM:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
    case Expression::Type::MODULUS:
    case Expression::Type::IF:
    case Expression::Type::FACTORIAL:
    case Expression::Type::EQUAL:
    case Expression::Type::NOT_EQUAL:
    case Expression::Type::LESS_EQUAL:
    case Expression::Type::GREATER_EQUAL:
      return compareChildren(e);
  }
  return 0;  // equal
}

bool Expression::contains(const Expression& e) const {
  if (*this == e) {
    return true;
  }
  return std::any_of(children.begin(), children.end(),
                     [&](const Expression& c) { return c.contains(e); });
}

bool Expression::contains(Type t) const {
  if (type == t) {
    return true;
  }
  return std::any_of(children.begin(), children.end(),
                     [&](const Expression& c) { return c.contains(t); });
}

bool Expression::contains(Type t, const std::string& name) const {
  if (type == t && this->name == name) {
    return true;
  }
  return std::any_of(children.begin(), children.end(),
                     [&](const Expression& c) { return c.contains(t, name); });
}

size_t Expression::numTerms() const {
  size_t result = 1;
  for (const auto& c : children) {
    result += c.numTerms();
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
    auto r = children[i].compare(e.children[i]);
    if (r != 0) {
      return r;
    }
  }
  return 0;  // equal
}

Expression& Expression::newChild(const Expression& e) {
  children.push_back(e);
  return children.back();
}

Expression& Expression::newChild(Expression::Type type, const std::string& name,
                                 const Number& value) {
  return children.emplace_back(Expression(type, name, value));
}

void Expression::replaceAll(const Expression& from, const Expression& to) {
  if (*this == from) {
    *this = to;
  } else {
    for (auto& c : children) {
      c.replaceAll(from, to);
    }
  }
}

void Expression::replaceInside(const Expression& from, const Expression& to,
                               const Expression::Type type) {
  for (auto& c : children) {
    if (this->type == type) {
      c.replaceAll(from, to);
    } else {
      c.replaceInside(from, to, type);
    }
  }
}

void Expression::replaceName(const std::string& from, const std::string& to) {
  if (name == from) {
    name = to;
  }
  for (auto& c : children) {
    c.replaceName(from, to);
  }
}

void Expression::replaceType(Type currentType, const std::string& matchName,
                             size_t arity, Type newType) {
  if (type == currentType && name == matchName && children.size() == arity) {
    type = newType;
    name.clear();
  }
  for (auto& child : children) {
    child.replaceType(currentType, matchName, arity, newType);
  }
}

std::ostream& operator<<(std::ostream& out, const Expression& e) {
  e.print(out, false, true, Expression::Type::CONSTANT);
  return out;
}

std::string Expression::toString(bool curry) const {
  std::stringstream ss;
  print(ss, curry, true, Expression::Type::CONSTANT);
  return ss.str();
}

std::pair<Expression, bool> extractSign(const Expression& e) {
  std::pair<Expression, bool> result;
  switch (e.type) {
    case Expression::Type::CONSTANT:
      result.first = e;
      if (e.value < Number::ZERO) {
        result.first.value.negate();
        result.second = true;
      } else {
        result.second = false;
      }
      break;
    case Expression::Type::PRODUCT:
      result.first.type = Expression::Type::PRODUCT;
      result.second = false;
      for (auto& c : e.children) {
        if (c.type == Expression::Type::CONSTANT && c.value < Number::ZERO) {
          auto constant = c;  // copy
          constant.value.negate();
          if (constant.value != Number::ONE) {
            result.first.newChild(constant);
          }
          result.second = !result.second;
        } else {
          result.first.newChild(c);
        }
      }
      if (result.first.children.empty()) {
        result.first.newChild(
            Expression(Expression::Type::CONSTANT, "", Number::ONE));
      }
      break;
    default:
      result.first = e;
      result.second = false;
      break;
  }
  return result;
}

void Expression::print(std::ostream& out, bool curry, bool isRoot,
                       Expression::Type parentType) const {
  const bool brackets = needsBrackets(isRoot, parentType, curry);
  if (brackets) {
    out << "(";
  }
  auto extracted = extractSign(*this);
  if (extracted.second) {
    out << "-";
  }
  extracted.first.printExtracted(out, curry);
  if (brackets) {
    out << ")";
  }
}

void Expression::printExtracted(std::ostream& out, bool curry) const {
  switch (type) {
    case Expression::Type::CONSTANT:
      out << value;
      break;
    case Expression::Type::PARAMETER:
      out << name;
      break;
    case Expression::Type::FUNCTION:
      if (curry) {
        printChildrenWrapped(out, curry, " ", name + " ", "");
      } else {
        printChildrenWrapped(out, curry, ",", name + "(", ")");
      }
      break;
    case Expression::Type::VECTOR:
      printChildrenWrapped(out, curry, ",", name + "[", "]");
      break;
    case Expression::Type::LOCAL:
      printChildrenWrapped(out, curry, "); ", "local(" + name + "=", "");
      break;
    case Expression::Type::SUM:
      printChildren(out, curry, "+");
      break;
    case Expression::Type::PRODUCT:
      printChildren(out, curry, "*");
      break;
    case Expression::Type::FRACTION:
      printChildren(out, curry, "/");
      break;
    case Expression::Type::POWER:
      printChildren(out, curry, "^");
      break;
    case Expression::Type::MODULUS:
      printChildren(out, curry, "%");
      break;
    case Expression::Type::IF:
      assertNumChildren(3);
      printChildrenWrapped(out, curry, ",", "if(n==", ")");
      break;
    case Expression::Type::FACTORIAL:
      assertNumChildren(1);
      children[0].print(out, curry, false, Expression::Type::FACTORIAL);
      out << "!";
      break;
    case Type::EQUAL:
    case Type::NOT_EQUAL:
    case Type::LESS_EQUAL:
    case Type::GREATER_EQUAL:
      assertNumChildren(2);
      printChildren(out, curry,
                    (type == Type::EQUAL        ? "=="
                     : type == Type::NOT_EQUAL  ? (curry ? " != " : "!=")
                     : type == Type::LESS_EQUAL ? "<="
                                                : ">="));
      break;
  }
}

bool Expression::needsBrackets(bool isRoot, Expression::Type parentType,
                               bool curry) const {
  if (isRoot) {
    return false;
  }
  if (parentType == Expression::Type::FUNCTION) {
    if (!curry) {
      return false;
    }
    // In curry mode, parameters don't need brackets
    if (type == Expression::Type::PARAMETER) {
      return false;
    }
    // Constants need brackets if they are negative (to avoid parsing ambiguity)
    if (type == Expression::Type::CONSTANT) {
      return value < Number::ZERO;
    }
    // All other expression types need brackets
    return true;
  }
  if (type == Expression::Type::PARAMETER) {
    return false;
  }
  if (type == Expression::Type::CONSTANT &&
      (parentType == Expression::Type::SUM || (Number(-1) < value))) {
    return false;
  }
  if (type == Expression::Type::FUNCTION) {
    return curry;
  }
  if (type == Expression::Type::VECTOR ||
      parentType == Expression::Type::VECTOR) {
    return false;
  }
  if (type == Expression::Type::LOCAL ||
      parentType == Expression::Type::LOCAL) {
    return false;
  }
  if (type == Expression::Type::IF || parentType == Expression::Type::IF) {
    return false;
  }
  if (type == Expression::Type::PRODUCT || type == Expression::Type::POWER ||
      type == Expression::Type::FRACTION || type == Expression::Type::MODULUS) {
    if (parentType == Expression::Type::SUM) {
      return false;
    }
  }
  if (type == Expression::Type::POWER &&
      parentType == Expression::Type::PRODUCT) {
    return false;
  }
  if (type == Expression::Type::FACTORIAL &&
      (parentType == Expression::Type::FACTORIAL ||
       parentType == Expression::Type::PRODUCT ||
       parentType == Expression::Type::SUM)) {
    return false;
  }
  if (parentType == Expression::Type::FACTORIAL && !isRoot) {
    if (!(type == Expression::Type::CONSTANT ||
          type == Expression::Type::PARAMETER ||
          type == Expression::Type::FUNCTION)) {
      return true;
    } else {
      return false;
    }
  }
  switch (type) {
    case Type::EQUAL:
    case Type::NOT_EQUAL:
    case Type::LESS_EQUAL:
    case Type::GREATER_EQUAL:
      // Comparison expressions need brackets unless at root or parent is IF
      return !isRoot && parentType != Type::IF;
    default:
      return true;
  }
}

void Expression::printChildren(std::ostream& out, bool curry,
                               const std::string& op) const {
  for (size_t i = 0; i < children.size(); i++) {
    auto extracted = extractSign(children[i]);
    if (i > 0 && (op != "+" || !extracted.second)) {
      out << op;
    }
    children[i].print(out, curry, false, type);
  }
}

void Expression::printChildrenWrapped(std::ostream& out, bool curry,
                                      const std::string& op,
                                      const std::string& prefix,
                                      const std::string& suffix) const {
  out << prefix;
  printChildren(out, curry, op);
  out << suffix;
}
