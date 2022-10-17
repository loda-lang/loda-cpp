#include "formula.hpp"

#include <stdexcept>

#include "log.hpp"
#include "program_util.hpp"

std::string Formula::toString() const {
  std::string result;
  for (auto& e : entries) {
    if (!result.empty()) {
      result += ", ";
    }
    result += e.first.toString() + "=" + e.second.toString();
  }
  return result;
}

Expression simpleFunction(const std::string& f, const std::string& p) {
  Expression e(Expression::Type::FUNCTION, f);
  e.newChild(Expression::Type::PARAMETER, p);
  return e;
}

Expression operandToExpression(Operand op) {
  switch (op.type) {
    case Operand::Type::CONSTANT: {
      return Expression(Expression::Type::CONSTANT, "", op.value);
    }
    case Operand::Type::DIRECT: {
      std::string a("a");
      if (op.value != Number::ZERO) {
        a += op.value.to_string();
      }
      return simpleFunction(a, "n");
    }
    case Operand::Type::INDIRECT: {
      throw std::runtime_error("indirect operation not supported");
    }
  }
  throw std::runtime_error("internal error");  // unreachable
}

std::pair<bool, Formula> Formula::fromProgram(const Program& p, bool pariMode) {
  std::pair<bool, Formula> result;
  result.first = false;
  Formula& f = result.second;

  // indirect operands are not supported
  if (ProgramUtil::hasIndirectOperand(p)) {
    return result;
  }

  // initialize expressions for memory cells
  Expression paramKey;  // initialized in the loop
  int64_t largestCell = ProgramUtil::getLargestDirectMemoryCell(p);
  for (int64_t i = 0; i <= largestCell; i++) {
    auto key = operandToExpression(Operand(Operand::Type::DIRECT, i));
    if (i == 0) {
      paramKey = key;
      f.entries[key] = Expression(Expression::Type::PARAMETER, "n");
    } else {
      f.entries[key] = Expression(Expression::Type::CONSTANT, "", Number::ZERO);
    }
  }

  // update expressions using operations
  for (auto& op : p.ops) {
    if (!f.update(op, pariMode)) {
      return result;  // operation not supported
    }
    Log::get().debug("Operation " + ProgramUtil::operationToString(op) +
                     " updated formula to " + f.toString());
  }

  auto r = f.entries[paramKey];
  f.entries.clear();
  f.entries[paramKey] = r;
  result.first = true;
  return result;
}

bool CanBeNegative(const Expression& e) {
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
      for (auto c : e.children) {
        if (CanBeNegative(*c)) {
          return true;
        }
      }
      return false;
  }
  return false;
}

Expression treeExpr(Expression::Type t, const Expression& c1,
                    const Expression& c2) {
  Expression result(t);
  result.newChild(c1);
  result.newChild(c2);
  result.normalize();
  return result;
}

Expression binFunc(const std::string& name, const Expression& c1,
                   const Expression& c2) {
  Expression func(Expression::Type::FUNCTION, name);
  func.newChild(c1);
  func.newChild(c2);
  return func;
}

Expression func(const Expression& e, const std::string& name) {
  auto f = Expression(Expression::Type::FUNCTION, name);
  f.newChild(e);
  return f;
}

Expression fraction(const Expression& num, const Expression& den,
                    bool pariMode) {
  auto frac = treeExpr(Expression::Type::FRACTION, num, den);
  if (pariMode) {
    if (CanBeNegative(num) || CanBeNegative(den)) {
      return func(frac, "truncate");
    } else {
      return func(frac, "floor");
    }
  }
  return frac;
}

bool Formula::update(const Operation& op, bool pariMode) {
  auto source = operandToExpression(op.source);
  auto target = operandToExpression(op.target);

  if (source.type == Expression::Type::FUNCTION) {
    source = entries[source];
  }
  auto prevTarget = entries[target];
  auto& result = entries[target];  // reference to result
  switch (op.type) {
    case Operation::Type::NOP:
      return true;
    case Operation::Type::MOV:
      result = source;
      return true;
    case Operation::Type::ADD:
      result = treeExpr(Expression::Type::SUM, prevTarget, source);
      return true;
    case Operation::Type::SUB:
      result = treeExpr(Expression::Type::DIFFERENCE, prevTarget, source);
      return true;
    case Operation::Type::MUL:
      result = treeExpr(Expression::Type::PRODUCT, prevTarget, source);
      return true;
    case Operation::Type::DIV: {
      result = fraction(prevTarget, source, pariMode);
      return true;
    }
    case Operation::Type::POW: {
      auto pow = treeExpr(Expression::Type::POWER, prevTarget, source);
      if (pariMode && CanBeNegative(source)) {
        result = func(pow, "truncate");
      } else {
        result = pow;
      }
      return true;
    }
    case Operation::Type::MOD: {
      if (pariMode && (CanBeNegative(prevTarget) || CanBeNegative(source))) {
        result = Expression(Expression::Type::DIFFERENCE);
        result.newChild(prevTarget);
        result.newChild(Expression::Type::PRODUCT);
        result.children[1]->newChild(source);
        result.children[1]->newChild(fraction(prevTarget, source, pariMode));
        return true;
      } else {
        result = treeExpr(Expression::Type::MODULUS, prevTarget, source);
        return true;
      }
    }
    case Operation::Type::MIN: {
      result = binFunc("min", prevTarget, source);
      return true;
    }
    case Operation::Type::MAX: {
      result = binFunc("max", prevTarget, source);
      return true;
    }
    default:
      return false;
  }
}

void Formula::replaceAll(const Expression& from, const Expression& to) {
  auto newEntries = entries;
  for (auto& e : entries) {
    auto key = e.first;
    auto value = e.second;
    key.replaceAll(from, to);
    value.replaceAll(from, to);
    newEntries[key] = value;
  }
  entries = newEntries;
}
