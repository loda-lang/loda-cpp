#include "expression_util.hpp"

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
  if (e.type != Expression::Type::SUM &&
      e.type != Expression::Type::DIFFERENCE &&
      e.type != Expression::Type::PRODUCT) {
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
    // for differences, we can only pull up the first child
    if (e.type == Expression::Type::DIFFERENCE) {
      break;
    }
  }
  e.children.insert(e.children.begin(), collected.begin(), collected.end());
  return !collected.empty();
}

bool simplifyNegativeProduct(Expression& e) {
  if (e.type != Expression::Type::PRODUCT) {
    return false;
  }
  if (e.children.size() != 2) {
    return false;
  }
  if (e.children[0]->type != Expression::Type::CONSTANT) {
    return false;
  }
  if (e.children[0]->value != Number(-1)) {
    return false;
  }
  auto tmp = *(e.children[1]);
  if (tmp.type == Expression::Type::DIFFERENCE && tmp.children.size() == 2) {
    std::swap(tmp.children[0], tmp.children[1]);
    e = tmp;
  } else {
    e = Expression(Expression::Type::NEGATION);
    e.newChild(tmp);
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
    case Expression::Type::DIFFERENCE:
      neutralElem = Number::ZERO;
      start = 1;
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
  } else if (e.children.size() == 1) {
    e = Expression(*e.children[0]);
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
    return true;
  }
  return false;
}

bool diffToNeg(Expression& e) {
  if (e.type == Expression::Type::DIFFERENCE && e.children.size() == 2 &&
      *(e.children.front()) ==
          Expression(Expression::Type::CONSTANT, "", Number::ZERO)) {
    auto c = *(e.children[1]);
    if (c.type == Expression::Type::NEGATION) {
      e = *(c.children[0]);
    } else {
      e = Expression(Expression::Type::NEGATION);
      e.newChild(c);
    }
    return true;
  }
  return false;
}

bool compareExprPtr(const Expression* lhs, const Expression* rhs) {
  return (*lhs < *rhs);
}

bool ExpressionUtil::normalize(Expression& e) {
  for (auto& c : e.children) {
    normalize(*c);
  }
  switch (e.type) {
    case Expression::Type::SUM:
      if (e.children.size() > 1) {  // at least two elements
        std::sort(e.children.rbegin(), e.children.rend(), compareExprPtr);
        mergeAllChildren(e);
      }
      break;
    case Expression::Type::PRODUCT:
      if (e.children.size() > 1) {  // at least two elements
        std::sort(e.children.begin(), e.children.end(), compareExprPtr);
        mergeAllChildren(e);
      }
      break;
    case Expression::Type::DIFFERENCE:
    case Expression::Type::FRACTION:
      if (e.children.size() > 2) {  // at least three elements
        std::sort(e.children.rbegin(), e.children.rend() - 1, compareExprPtr);
        mergeAllChildren(e);
      }
      break;
    default:
      break;
  }
  if (pullUpChildren(e)) {
    normalize(e);
  }
  diffToNeg(e);
  removeNeutral(e);
  zeroProduct(e);
  simplifyNegativeProduct(e);
  // TODO: track changes
  return true;
}

bool ExpressionUtil::canBeNegative(const Expression& e) {
  switch (e.type) {
    case Expression::Type::CONSTANT:
      return e.value < Number::ZERO;
    case Expression::Type::PARAMETER:
      return false;
    case Expression::Type::FUNCTION:
    case Expression::Type::NEGATION:
    case Expression::Type::DIFFERENCE:
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
