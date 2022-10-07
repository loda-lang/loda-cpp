#include "formula.hpp"

#include "evaluator_inc.hpp"

std::string Formula::to_string() const {
  std::string result;
  for (size_t i = 0; i < entries.size(); i++) {
    if (i > 0) {
      result += ", ";
    }
    auto& e = entries[i];
    result += e.first.toString() + "=" + e.second.toString();
  }
  return result;
}

Expression* Formula::findEndtry(const Expression& left) {
  for (auto& e : entries) {
    if (e.first == left) {
      return &e.second;
    }
  }
  return nullptr;
}

Expression operandToExpression(Operand op, bool prev) {
  Expression e;
  if (op.type == Operand::Type::DIRECT) {
    e.type = Expression::Type::FUNCTION;
    e.name = "a" + op.value.to_string();
    if (prev) {
      Expression d(Expression::Type::DIFFERENCE);
      d.newChild(Expression::Type::PARAMETER, "n");
      d.newChild(Expression::Type::CONSTANT, "", Number(1));
      e.newChild(d);
    } else {
      e.newChild(Expression::Type::PARAMETER, "n");
    }
  } else {
    e.type = Expression::Type::CONSTANT;
    e.value = op.value;
  }
  return e;
}

std::pair<bool, Formula> Formula::fromProgram(const Program& p) {
  std::pair<bool, Formula> result;
  result.first = false;

  // TODO: pass in settings, cache as members?
  Settings settings;
  Interpreter interpreter(settings);
  IncrementalEvaluator evaluator(interpreter);

  if (!evaluator.init(p)) {
    return result;
  }

  auto& rec = result.second;
  auto& body = evaluator.getLoopBody();
  for (auto& op : body.ops) {
    std::pair<Expression, Expression> e;

    const Expression targetExpr = operandToExpression(op.target, false);
    e.first = targetExpr;

    if (op.type == Operation::Type::MOV) {
      e.second = operandToExpression(op.source, false);
    } else if (op.type == Operation::Type::ADD) {
      e.second.type = Expression::Type::SUM;

      auto f = rec.findEndtry(targetExpr);
      if (f) {
        e.second.newChild(targetExpr);
      } else {
        e.second.newChild(operandToExpression(op.target, true));
      }

      e.second.newChild(operandToExpression(op.source, false));
    }

    rec.entries.emplace_back(std::move(e));
  }

  result.first = true;
  return result;
}
