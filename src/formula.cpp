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
    Log::get().debug("Operation " + ProgramUtil::operationToString(op) +
                     " updated formula to " + f.toString());
  }
  if (!ok) {
    return std::pair<bool, Formula>(false, {});
  }
  auto expr = f.find(paramKey);
  f.entries.clear();
  f.entries[paramKey] = expr;
  return std::pair<bool, Formula>(true, f);
}

Expression Formula::find(const Expression& e) {
  auto it = entries.find(e);
  if (it != entries.end()) {
    return it->second;
  }
  return Expression(Expression::Type::CONSTANT, "", 0);
}

Expression treeExpr(Expression::Type t, const Expression& c1,
                    const Expression& c2) {
  Expression result(t);
  result.newChild(c1);
  result.newChild(c2);
  result.normalize();
  return result;
}

bool Formula::update(const Operation& op) {
  auto source = operandToExpression(op.source);
  auto target = operandToExpression(op.target);
  auto prevTargetValue = find(target);
  switch (op.type) {
    case Operation::Type::NOP:
      return true;
    case Operation::Type::MOV:
      entries[target] = source;
      return true;
    case Operation::Type::ADD:
      entries[target] =
          treeExpr(Expression::Type::SUM, prevTargetValue, source);
      return true;
    case Operation::Type::SUB:
      entries[target] =
          treeExpr(Expression::Type::DIFFERENCE, prevTargetValue, source);
      return true;
    case Operation::Type::MUL:
      entries[target] =
          treeExpr(Expression::Type::PRODUCT, prevTargetValue, source);
      return true;
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
