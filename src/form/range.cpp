#include "form/range.hpp"

Range& Range::operator+=(const Range& r) {
  lower_bound += r.lower_bound;
  upper_bound += r.upper_bound;
  return *this;
}

void RangeSet::prune() {
  for (auto it = begin(); it != end();) {
    if (it->second.lower_bound == Number::INF &&
        it->second.upper_bound == Number::INF) {
      it = erase(it);
    } else {
      ++it;
    }
  }
}

std::string RangeSet::toString() const {
  std::string result;
  for (const auto& it : *this) {
    const auto& r = it.second;
    if (r.lower_bound == Number::INF && r.upper_bound == Number::INF) {
      continue;
    }
    if (!result.empty()) {
      result += ", ";
    }
    if (r.lower_bound == r.upper_bound) {
      result +=
          "$" + std::to_string(it.first) + " = " + r.lower_bound.to_string();
    } else {
      if (r.lower_bound != Number::INF) {
        result += r.lower_bound.to_string() + " <= ";
      }
      result += "$" + std::to_string(it.first);
      if (r.upper_bound != Number::INF) {
        result += " <= " + r.upper_bound.to_string();
      }
    }
  }
  return result;
}
