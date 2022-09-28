#include "recurrence.hpp"

#include "evaluator_inc.hpp"

std::string Expression::to_string() const {
  switch (type) {
    case Expression::Type::CONSTANT:
      return value.to_string();
    case Expression::Type::PARAMETER:
      return name;
    case Expression::Type::PLUS:
      return to_string_op("+");
    case Expression::Type::MINUS:
      return to_string_op("-");
    case Expression::Type::MUL:
      return to_string_op("*");
    case Expression::Type::DIV:
      return to_string_op("/");
    case Expression::Type::RECURRENCE:
      return name + "(" + children.at(0)->to_string() + ")";
  }
}

std::string Expression::to_string_op(const std::string& op) const {
  std::string out;
  for (size_t i = 0; i < children.size(); i++) {
    if (i > 0) {
      out += op;
    }
    out += children[i]->to_string();
  }
  return out;
}

void Expression::addChild(Expression::Type type, const std::string& name,
                          Number value) {
  children.emplace_back(std::unique_ptr<Expression>(new Expression()));
  children.back()->type = type;
  children.back()->name = name;
  children.back()->value = value;
}

std::string RecurrenceRelation::to_string() const {
  std::string result;
  for (size_t i = 0; i < entries.size(); i++) {
    if (i > 0) {
      result += ", ";
    }
    auto& e = entries[i];
    result += e.first.to_string() + "=" + e.second.to_string();
  }
  return result;
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

    if (op.type == Operation::Type::MOV) {
      e.first.type = Expression::Type::RECURRENCE;
      e.first.name = "a" + op.target.value.to_string();
      e.first.addChild(Expression::Type::PARAMETER, "n");
      if (op.source.type == Operand::Type::DIRECT) {
        e.second.type = Expression::Type::RECURRENCE;
        e.second.name = "a" + op.source.value.to_string();
        e.second.addChild(Expression::Type::PARAMETER, "n");
      }
      rec.entries.emplace_back(std::move(e));
    } else if (op.type == Operation::Type::ADD) {
    }
  }

  result.first = true;
  return result;
}
