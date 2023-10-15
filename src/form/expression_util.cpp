#include "form/expression_util.hpp"

Expression ExpressionUtil::newConstant(int64_t value) {
  return Expression(Expression::Type::CONSTANT, "", Number(value));
}

Expression ExpressionUtil::newParameter() {
  return Expression(Expression::Type::PARAMETER, "n");
}

Expression ExpressionUtil::newFunction(const std::string& name) {
  return Expression(Expression::Type::FUNCTION, name, {newParameter()});
}

bool mergeTwoChildren(Expression& e) {
  for (size_t i = 0; i + 1 < e.children.size(); i++) {
    auto c = e.children[i];
    auto d = e.children[i + 1];
    bool merged = false;
    if (c->type == Expression::Type::CONSTANT &&
        d->type == Expression::Type::CONSTANT) {
      switch (e.type) {
        case Expression::Type::SUM: {
          c->value += d->value;
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
      switch (e.type) {
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
      e.children.erase(e.children.begin() + i + 1);
      return true;
    }
  }
  return false;
}

bool mergeAllChildren(Expression& e) {
  bool changed = false;
  while (mergeTwoChildren(e)) {
    changed = true;
  }
  return changed;
}

bool pullUpChildren(Expression& e) {
  if (e.type != Expression::Type::SUM && e.type != Expression::Type::PRODUCT) {
    return false;
  }
  std::vector<Expression*> collected;
  auto it = e.children.begin();
  while (it != e.children.end()) {
    if ((*it)->type == e.type) {
      collected.insert(collected.end(), (*it)->children.begin(),
                       (*it)->children.end());
      it = e.children.erase(it);
    } else {
      it++;
    }
  }
  e.children.insert(e.children.begin(), collected.begin(), collected.end());
  return !collected.empty();
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
