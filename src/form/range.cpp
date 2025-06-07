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

void updateMinMax(const Number& val, Number& min, Number& max) {
  if (val < min) min = val;
  if (val > max) max = val;
}

Range& Range::operator*=(const Range& r) {
  auto l1 = lower_bound;
  auto l2 = r.lower_bound;
  auto u1 = upper_bound;
  auto u2 = r.upper_bound;
  if (isFinite() && r.isFinite()) {
    // both finite
    auto a1 = Semantics::mul(l1, l2);
    auto a2 = Semantics::mul(l1, u2);
    auto a3 = Semantics::mul(u1, l2);
    auto a4 = Semantics::mul(u1, u2);
    lower_bound = a1;
    upper_bound = a1;
    updateMinMax(a2, lower_bound, upper_bound);
    updateMinMax(a3, lower_bound, upper_bound);
    updateMinMax(a4, lower_bound, upper_bound);
  } else {
    // at least one is infinite
    if (l1 >= 0 && l2 >= 0) {
      lower_bound = Semantics::mul(l1, l2);
    } else if (u1 <= 0 && u2 <= 0) {
      lower_bound = Semantics::mul(u1, u2);
    } else {
      lower_bound = Number::INF;
    }
    if (l1 >= 0 && u2 <= 0) {
      upper_bound = Semantics::mul(l1, u2);
    } else if (l2 >= 0 && u1 <= 0) {
      upper_bound = Semantics::mul(l2, u1);
    } else if (u1 >= 0 && u2 >= 0 && (l1 >= 0 || l2 >= 0)) {
      upper_bound = Semantics::mul(u1, u2);
    } else {
      upper_bound = Number::INF;
    }
  }
  return *this;
}

Range& Range::operator%=(const Range& r) {
  // TODO: suport more cases
  if (lower_bound >= 0) {
    lower_bound = 0;
    if (r.upper_bound != Number::INF) {
      auto abs_lower = Semantics::abs(r.lower_bound);
      auto abs_upper = Semantics::abs(r.upper_bound);
      auto max_abs = Semantics::max(abs_lower, abs_upper);
      upper_bound = Semantics::sub(max_abs, 1);
    } else {
      upper_bound = Number::INF;
    }
  } else {
    lower_bound = Number::INF;
    upper_bound = Number::INF;
  }
  return *this;
}

Range& Range::gcd(const Range& r) {
  auto l = Number::ZERO;
  auto u = Number::INF;
  if (lower_bound != Number::INF && lower_bound >= Number::ONE) {
    l = Number::ONE;
    if (u == Number::INF || u > upper_bound) {
      u = upper_bound;
    }
  }
  if (r.lower_bound != Number::INF && r.lower_bound >= Number::ONE) {
    l = Number::ONE;
    if (u == Number::INF || u > r.upper_bound) {
      u = r.upper_bound;
    }
  }
  lower_bound = l;
  upper_bound = u;
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

int64_t Range::check(const Sequence& seq) const {
  int64_t numTerms = seq.size();
  for (int64_t i = 0; i < numTerms; ++i) {
    if (seq[i] < lower_bound || seq[i] > upper_bound) {
      return i;
    }
  }
  return -1;
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
    result += toString(it.first);
  }
  return result;
}

std::string RangeMap::toString(int64_t index) const {
  auto it = find(index);
  std::string result;
  if (it != end()) {
    auto& r = it->second;
    if (r.isUnbounded()) {
      return result;
    }
    if (r.isConstant()) {
      result += "$" + std::to_string(index) + " = " + r.lower_bound.to_string();
    } else {
      if (r.lower_bound != Number::INF) {
        result += r.lower_bound.to_string() + " <= ";
      }
      result += "$" + std::to_string(index);
      if (r.upper_bound != Number::INF) {
        result += " <= " + r.upper_bound.to_string();
      }
    }
  }
  return result;
}
