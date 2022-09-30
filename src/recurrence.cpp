#include "recurrence.hpp"

#include "evaluator_inc.hpp"

std::string RecurrenceRelation::to_string() const {
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

Expression operandToExpression(Operand op) {
  Expression e;
  if (op.type == Operand::Type::DIRECT) {
    e.type = Expression::Type::FUNCTION;
    e.name = "a" + op.value.to_string();
    e.newChild(Expression::Type::PARAMETER, "n");
  } else {
    e.type = Expression::Type::CONSTANT;
    e.value = op.value;
  }
  return e;
}

std::pair<bool, RecurrenceRelation> RecurrenceRelation::fromProgram(
    const Program& p) {
  std::pair<bool, RecurrenceRelation> result;
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

    e.first.type = Expression::Type::FUNCTION;
    e.first.name = "a" + op.target.value.to_string();
    e.first.newChild(Expression::Type::PARAMETER, "n");

    if (op.type == Operation::Type::MOV) {
      e.second = operandToExpression(op.source);
    } else if (op.type == Operation::Type::ADD) {
      e.second.type = Expression::Type::SUM;

      auto t = new Expression();
      t->type = Expression::Type::FUNCTION;
      t->name = "a" + op.target.value.to_string();
      t->newChild(Expression::Type::PARAMETER, "n");

      e.second.children.push_back(t);
      e.second.children.push_back(
          new Expression(operandToExpression(op.source)));
    }

    rec.entries.emplace_back(std::move(e));
  }

  result.first = true;
  return result;
}
