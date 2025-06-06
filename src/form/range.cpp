#include "form/range.hpp"

#include "eval/semantics.hpp"

// Helper function to update min and max
static void update_min_max(Number val, Number& min, Number& max) {
  if (val < min) min = val;
  if (val > max) max = val;
}

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
  // TODO: support more cases
  auto l = Number::INF;
  auto u = Number::INF;
  if (isFinite() && r.isFinite()) {
    auto a1 = Semantics::mul(lower_bound, r.lower_bound);
    auto a2 = Semantics::mul(lower_bound, r.upper_bound);
    auto a3 = Semantics::mul(upper_bound, r.lower_bound);
    auto a4 = Semantics::mul(upper_bound, r.upper_bound);
    l = a1;
    u = a1;
    update_min_max(a2, l, u);
    update_min_max(a3, l, u);
    update_min_max(a4, l, u);
  } else {
    if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
        r.lower_bound != Number::INF && r.lower_bound >= Number::ZERO) {
      l = Semantics::mul(lower_bound, r.lower_bound);
    }
    if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
        r.upper_bound != Number::INF && r.upper_bound <= Number::ZERO) {
      u = Semantics::mul(lower_bound, r.upper_bound);
    }
    if (upper_bound != Number::INF && upper_bound >= Number::ZERO &&
        r.upper_bound != Number::INF && r.upper_bound >= Number::ZERO) {
      u = Semantics::mul(upper_bound, r.upper_bound);
    }
  }
  lower_bound = l;
  upper_bound = u;
  return *this;
}

Range& Range::operator%=(const Range& r) {
  // TODO: suport more cases
  bool updated = false;
  if (lower_bound != Number::INF && lower_bound >= Number::ZERO) {
    lower_bound = Number::ZERO;
    if (r.upper_bound != Number::INF) {
      auto abs_lower = Semantics::abs(r.lower_bound);
      auto abs_upper = Semantics::abs(r.upper_bound);
      auto max_abs = Semantics::max(abs_lower, abs_upper);
      upper_bound = Semantics::sub(max_abs, Number::ONE);
    } else {
      upper_bound = Number::INF;
    }
    updated = true;
  }
  if (!updated) {
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
    if ((lower_bound != Number::INF && seq[i] < lower_bound) ||
        (upper_bound != Number::INF && seq[i] > upper_bound)) {
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
