#include "formula.hpp"

#include <stdexcept>

#include "expression_util.hpp"
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
      return Expression(Expression::Type::FUNCTION, a,
                        {Expression(Expression::Type::PARAMETER, "n")});
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

Expression fraction(const Expression& num, const Expression& den,
                    bool pariMode) {
  auto frac = Expression(Expression::Type::FRACTION, "", {num, den});
  if (pariMode) {
    std::string func = "floor";
    if (ExpressionUtil::canBeNegative(num) ||
        ExpressionUtil::canBeNegative(den)) {
      func = "truncate";
    }
    return Expression(Expression::Type::FUNCTION, func, {frac});
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
  auto& res = entries[target];  // reference to result
  bool okay = true;
  switch (op.type) {
    case Operation::Type::NOP: {
      break;
    }
    case Operation::Type::MOV: {
      res = source;
      break;
    }
    case Operation::Type::ADD: {
      res = Expression(Expression::Type::SUM, "", {prevTarget, source});
      break;
    }
    case Operation::Type::SUB: {
      res = Expression(Expression::Type::DIFFERENCE, "", {prevTarget, source});
      break;
    }
    case Operation::Type::MUL: {
      res = Expression(Expression::Type::PRODUCT, "", {prevTarget, source});
      break;
    }
    case Operation::Type::DIV: {
      res = fraction(prevTarget, source, pariMode);
      break;
    }
    case Operation::Type::POW: {
      auto pow = Expression(Expression::Type::POWER, "", {prevTarget, source});
      if (pariMode && ExpressionUtil::canBeNegative(source)) {
        res = Expression(Expression::Type::FUNCTION, "truncate", {pow});
      } else {
        res = pow;
      }
      break;
    }
    case Operation::Type::MOD: {
      if (pariMode && (ExpressionUtil::canBeNegative(prevTarget) ||
                       ExpressionUtil::canBeNegative(source))) {
        res = Expression(Expression::Type::DIFFERENCE);
        res.newChild(prevTarget);
        res.newChild(Expression::Type::PRODUCT);
        res.children[1]->newChild(source);
        res.children[1]->newChild(fraction(prevTarget, source, pariMode));
      } else {
        res = Expression(Expression::Type::MODULUS, "", {prevTarget, source});
      }
      break;
    }
    case Operation::Type::MIN: {
      res = Expression(Expression::Type::FUNCTION, "min", {prevTarget, source});
      break;
    }
    case Operation::Type::MAX: {
      res = Expression(Expression::Type::FUNCTION, "max", {prevTarget, source});
      break;
    }
    default: {
      okay = false;
      break;
    }
  }
  ExpressionUtil::normalize(res);
  return okay;
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
