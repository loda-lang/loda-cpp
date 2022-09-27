#include "recurrence.hpp"

std::string Expression::to_string() const {
  switch (type) {
    case Expression::Type::CONSTANT:
      return value.to_string();
    case Expression::Type::PARAMETER:
      return "n";
    case Expression::Type::PLUS:
      return left->to_string() + "+" + right->to_string();
    case Expression::Type::MINUS:
      return left->to_string() + "-" + right->to_string();
    case Expression::Type::MUL:
      return left->to_string() + "*" + right->to_string();
    case Expression::Type::DIV:
      return left->to_string() + "/" + right->to_string();
    case Expression::Type::RECURRENCE:
      return "a(" + left->to_string() + ")";
  }
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

  return result;
}
