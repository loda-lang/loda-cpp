#include "form/range.hpp"

#include "eval/semantics.hpp"

Range& Range::operator+=(const Range& r) {
  lower_bound += r.lower_bound;
  upper_bound += r.upper_bound;
  return *this;
}

Range& Range::operator-=(const Range& r) {
  lower_bound -= r.upper_bound;
  upper_bound -= r.lower_bound;
  return *this;
}

Range& Range::operator*=(const Range& r) {
  // TODO: suport more cases
  auto l = Number::INF;
  auto u = Number::INF;
  if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
      r.lower_bound != Number::INF && r.lower_bound >= Number::ZERO) {
    l = Semantics::mul(lower_bound, r.lower_bound);
  }
  if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
      r.lower_bound != Number::INF && r.lower_bound <= Number::ZERO) {
    u = Semantics::mul(lower_bound, r.lower_bound);
  }
  lower_bound = l;
  upper_bound = u;
  return *this;
}

Range& Range::operator%=(const Range& r) {
  // TODO: suport more cases
  if (r.isFinite()) {
    auto abs_lower = Semantics::abs(r.lower_bound);
    auto abs_upper = Semantics::abs(r.upper_bound);
    auto max_abs = Semantics::max(abs_lower, abs_upper);
    if (lower_bound != Number::INF && lower_bound >= Number::ZERO) {
      lower_bound = Number::ZERO;
      upper_bound = Semantics::sub(max_abs, Number::ONE);
    } else {
      lower_bound = Number::INF;
      upper_bound = Number::INF;
    }
  }
  return *this;
}

bool Range::isFinite() const {
  return lower_bound != Number::INF && upper_bound != Number::INF;
}

bool Range::isConstant() const {
  return isFinite() && lower_bound == upper_bound;
}

bool Range::isUnbounded() const {
  return lower_bound == Number::INF && upper_bound == Number::INF;
}

void RangeMap::prune() {
  for (auto it = begin(); it != end();) {
    if (it->second.isUnbounded()) {
      it = erase(it);
    } else {
      ++it;
    }
  }
}

std::string RangeMap::toString() const {
  std::string result;
  for (const auto& it : *this) {
    const auto& r = it.second;
    if (r.isUnbounded()) {
      continue;
    }
    if (!result.empty()) {
      result += ", ";
    }
    if (r.isConstant()) {
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
