#include "form/expression_util.hpp"

#include <stdexcept>

#include "lang/semantics.hpp"

Expression ExpressionUtil::newConstant(int64_t value) {
  return Expression(Expression::Type::CONSTANT, "", Number(value));
}

Expression ExpressionUtil::newParameter() {
  return Expression(Expression::Type::PARAMETER, "n");
}

Expression ExpressionUtil::newFunction(const std::string& name) {
  return Expression(Expression::Type::FUNCTION, name, {newParameter()});
}

std::pair<Number, Expression> extractFactor(const Expression& e) {
  std::pair<Number, Expression> result;
  result.first = Number::ONE;
  result.second.type = Expression::Type::PRODUCT;
  if (e.type == Expression::Type::PRODUCT) {
    for (auto c : e.children) {
      if (c->type == Expression::Type::CONSTANT) {
        result.first *= c->value;
      } else {
        result.second.newChild(*c);
      }
    }
  } else {
    result.second.newChild(e);
  }
  return result;
}

bool mergeSum(Expression& c, Expression& d) {
  if (c.type == Expression::Type::CONSTANT &&
      d.type == Expression::Type::CONSTANT) {
    c.value += d.value;
    d.value = Number::ZERO;
    return true;
  } else {
    auto p1 = extractFactor(c);
    auto p2 = extractFactor(d);
    if (p1.second == p2.second) {
      auto factor = Expression(Expression::Type::CONSTANT, "", p1.first);
      factor.value += p2.first;
      auto term = p1.second;
      if (p1.second.children.size() == 1) {  // we know it's a product
        term = *p1.second.children[0];
      }
      if (factor.value == Number::ZERO) {
        c = factor;
      } else if (factor.value == Number::ONE) {
        c = term;
      } else {
        c = Expression(Expression::Type::PRODUCT, "", {factor, term});
      }
      d = ExpressionUtil::newConstant(0);
      return true;
    }
  }
  return false;
}

bool mergeProduct(Expression& c, Expression& d) {
  if (c.type == Expression::Type::CONSTANT &&
      d.type == Expression::Type::CONSTANT) {
    c.value *= d.value;
    d.value = Number::ONE;
    return true;
  } else if (c == d) {
    c = Expression(Expression::Type::POWER);
    c.newChild(d);
    c.newChild(ExpressionUtil::newConstant(2));
    d.value = Number::ONE;
    return true;
  } else if (d.type == Expression::Type::POWER && d.children.size() == 2 &&
             d.children[1]->type == Expression::Type::CONSTANT &&
             c == *d.children[0]) {
    c = d;
    c.children[1]->value += 1;
    d = ExpressionUtil::newConstant(1);
    return true;
  } else if (c.type == Expression::Type::POWER && c.children.size() == 2 &&
             c.children[1]->type == Expression::Type::CONSTANT &&
             d.type == Expression::Type::POWER && d.children.size() == 2 &&
             d.children[1]->type == Expression::Type::CONSTANT &&
             *c.children[0] == *d.children[0]) {
    c.children[1]->value += d.children[1]->value;
    d = ExpressionUtil::newConstant(1);
    return true;
  }
  return false;
}

bool mergeAllChildren(Expression& e) {
  bool changed = false;
  for (size_t i = 0; i < e.children.size(); i++) {
    for (size_t j = 0; j < e.children.size(); j++) {
      if (i == j) {
        continue;
      }
      bool merged = false;
      if (e.type == Expression::Type::SUM) {
        merged = mergeSum(*e.children[i], *e.children[j]);
      } else if (e.type == Expression::Type::PRODUCT) {
        merged = mergeProduct(*e.children[i], *e.children[j]);
      }
      if (merged) {
        delete e.children[j];
        e.children.erase(e.children.begin() + j);
        changed = true;
        i = j = 0;
      }
    }
  }
  return changed;
}

bool pullUpChildren(Expression& e) {
  if (e.type != Expression::Type::SUM && e.type != Expression::Type::PRODUCT) {
    return false;
  }
  Expression result(e.type, e.name);
  bool changed = false;
  for (auto c : e.children) {
    if (c->type == e.type) {
      for (auto d : c->children) {
        result.newChild(*d);
      }
      changed = true;
    } else {
      result.newChild(*c);
    }
  }
  if (changed) {
    e = result;
  }
  return changed;
}

bool multiplyThrough(Expression& e) {
  if (e.type != Expression::Type::PRODUCT) {
    return false;
  }
  if (e.children.size() != 2) {
    return false;
  }
  if (e.children[0]->type != Expression::Type::CONSTANT) {
    return false;
  }
  if (e.children[1]->type != Expression::Type::SUM) {
    return false;
  }
  auto constant = *e.children[0];
  auto sum = *e.children[1];
  e.type = Expression::Type::SUM;
  e.children.clear();
  for (auto c : sum.children) {
    Expression prod(Expression::Type::PRODUCT, "", {constant});
    if (c->type == Expression::Type::PRODUCT) {
      for (auto d : c->children) {
        prod.newChild(*d);
      }
    } else {
      prod.newChild(*c);
    }
    e.newChild(prod);
  }
  return true;
}

bool removeNeutral(Expression& e) {
  Number neutralElem;
  size_t start = 0;
  switch (e.type) {
    case Expression::Type::SUM:
      neutralElem = Number::ZERO;
      start = 0;
      break;
    case Expression::Type::PRODUCT:
      neutralElem = Number::ONE;
      start = 0;
      break;
    case Expression::Type::FRACTION:
      neutralElem = Number::ONE;
      start = 1;
      break;
    default:
      return false;
  }
  Expression neutralExpr(Expression::Type::CONSTANT, "", neutralElem);
  auto it = e.children.begin() + start;
  bool changed = false;
  while (it != e.children.end()) {
    if (*(*it) == neutralExpr) {
      delete *it;
      it = e.children.erase(it);
      changed = true;
    } else {
      it++;
    }
  }
  if (e.children.empty()) {
    e = neutralExpr;
    changed = true;
  } else if (e.children.size() == 1) {
    e = Expression(*e.children[0]);
    changed = true;
  }
  return changed;
}

bool zeroProduct(Expression& e) {
  if (e.type != Expression::Type::PRODUCT) {
    return false;
  }
  const Expression zero(Expression::Type::CONSTANT, "", Number::ZERO);
  bool found = false;
  for (auto c : e.children) {
    if (*c == zero) {
      found = true;
      break;
    }
  }
  if (found) {
    e = zero;
  }
  return found;
}

bool lessExprPtr(const Expression* lhs, const Expression* rhs) {
  return *lhs < *rhs;
}

bool greaterExprPtr(const Expression* lhs, const Expression* rhs) {
  return *lhs > *rhs;
}

bool ExpressionUtil::normalize(Expression& e) {
  for (const auto& c : e.children) {
    normalize(*c);
  }
  switch (e.type) {
    case Expression::Type::SUM:
      if (e.children.size() > 1) {  // at least two elements
        std::sort(e.children.begin(), e.children.end(), greaterExprPtr);
        mergeAllChildren(e);
      }
      break;
    case Expression::Type::PRODUCT:
      if (e.children.size() > 1) {  // at least two elements
        std::sort(e.children.begin(), e.children.end(), lessExprPtr);
        mergeAllChildren(e);
      }
      break;
    case Expression::Type::FRACTION:
      if (e.children.size() > 2) {  // at least three elements
        std::sort(e.children.begin() + 1, e.children.end(), greaterExprPtr);
        mergeAllChildren(e);
      }
      break;
    default:
      break;
  }
  if (pullUpChildren(e)) {
    normalize(e);
  }
  removeNeutral(e);
  zeroProduct(e);
  multiplyThrough(e);
  // TODO: track changes
  return true;
}

bool ExpressionUtil::isSimpleFunction(const Expression& e, bool strict) {
  if (e.type != Expression::Type::FUNCTION || e.children.size() != 1) {
    return false;
  }
  const auto arg = e.children.front();
  if (strict) {
    return arg->type == Expression::Type::PARAMETER;
  } else {
    return !arg->contains(Expression::Type::FUNCTION);
  }
}

bool ExpressionUtil::canBeNegative(const Expression& e) {
  switch (e.type) {
    case Expression::Type::CONSTANT:
      return e.value < Number::ZERO;
    case Expression::Type::PARAMETER:
      return false;
    case Expression::Type::FUNCTION:
    case Expression::Type::LOCAL:
      return true;
    case Expression::Type::SUM:
    case Expression::Type::PRODUCT:
    case Expression::Type::FRACTION:
    case Expression::Type::POWER:
    case Expression::Type::MODULUS:
    case Expression::Type::IF:
      for (auto c : e.children) {
        if (canBeNegative(*c)) {
          return true;
        }
      }
      return false;
  }
  return false;
}

void ExpressionUtil::collectNames(const Expression& e, Expression::Type type,
                                  std::set<std::string>& target) {
  if (e.type == type) {
    target.insert(e.name);
  }
  for (auto c : e.children) {
    collectNames(*c, type, target);
  }
}

void assertNumChildren(const Expression& e, size_t num) {
  if (e.children.size() != num) {
    throw std::runtime_error("unexpected number of terms in " + e.toString());
  }
}

Number ExpressionUtil::eval(const Expression& e,
                            const std::map<std::string, Number> params) {
  switch (e.type) {
    case Expression::Type::CONSTANT: {
      return e.value;
    }
    case Expression::Type::PARAMETER: {
      return params.at(e.name);
    }
    case Expression::Type::SUM: {
      auto result = Number::ZERO;
      for (auto c : e.children) {
        result = Semantics::add(result, eval(*c, params));
      }
      return result;
    }
    case Expression::Type::PRODUCT: {
      auto result = Number::ONE;
      for (auto c : e.children) {
        result = Semantics::mul(result, eval(*c, params));
      }
      return result;
    }
    case Expression::Type::FRACTION: {
      assertNumChildren(e, 2);
      auto a = eval(*e.children[0], params);
      auto b = eval(*e.children[1], params);
      return Semantics::div(a, b);
    }
    case Expression::Type::POWER: {
      assertNumChildren(e, 2);
      auto a = eval(*e.children[0], params);
      auto b = eval(*e.children[1], params);
      return Semantics::pow(a, b);
    }
    case Expression::Type::MODULUS: {
      assertNumChildren(e, 2);
      auto a = eval(*e.children[0], params);
      auto b = eval(*e.children[1], params);
      return Semantics::mod(a, b);
    }
    default:
      throw std::runtime_error("cannot evaluate " + e.toString());
  }
}
