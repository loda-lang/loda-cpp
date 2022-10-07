#include "formula.hpp"

#include <stdexcept>

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
    case Operand::Type::CONSTANT:
      return Expression(Expression::Type::CONSTANT, "", op.value);
    case Operand::Type::DIRECT:
      return simpleFunction("a" + op.value.to_string(), "n");
    case Operand::Type::INDIRECT:
      throw std::runtime_error("indirect operation not supported");
  }
}

std::pair<bool, Formula> Formula::fromProgram(const Program& p) {
  const auto paramKey = operandToExpression(Operand(Operand::Type::DIRECT, 0));
  bool ok = true;
  Formula f;
  f.entries[paramKey] = Expression(Expression::Type::PARAMETER, "n");
  for (auto& op : p.ops) {
    if (!f.update(op)) {
      ok = false;
      break;
    }
  }
  if (!ok) {
    return std::pair<bool, Formula>(false, {});
  }
  auto expr = f.find(paramKey);
  auto finalKey = simpleFunction("a", "n");
  f.entries.clear();
  f.entries[finalKey] = expr;
  return std::pair<bool, Formula>(true, f);
}

Expression Formula::find(const Expression& e) {
  auto it = entries.find(e);
  if (it != entries.end()) {
    return it->second;
  }
  return Expression(Expression::Type::CONSTANT, "", 0);
}

bool Formula::update(const Operation& op) {
  auto source = operandToExpression(op.source);
  auto target = operandToExpression(op.target);
  switch (op.type) {
    case Operation::Type::NOP:
      return true;
    case Operation::Type::MOV:
      entries[target] = source;
      return true;
    default:
      return false;
  }
}
