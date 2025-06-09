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

// Helper to find min and max from a list of candidates
static void findMinMax(const Number* candidates, size_t n, Number& min,
                       Number& max) {
  min = candidates[0];
  max = candidates[0];
  for (size_t i = 1; i < n; ++i) {
    if (candidates[i] < min) min = candidates[i];
    if (candidates[i] > max) max = candidates[i];
  }
}

Range& Range::operator*=(const Range& r) {
  const auto l1 = lower_bound;
  const auto l2 = r.lower_bound;
  const auto u1 = upper_bound;
  const auto u2 = r.upper_bound;
  if (isFinite() && r.isFinite()) {  // both arguments are finite
    Number candidates[4] = {Semantics::mul(l1, l2), Semantics::mul(l1, u2),
                            Semantics::mul(u1, l2), Semantics::mul(u1, u2)};
    findMinMax(candidates, 4, lower_bound, upper_bound);
  } else {  // at least one argument is infinite
    // update lower bound
    if (l1 >= Number::ZERO && l2 >= Number::ZERO) {
      lower_bound = Semantics::mul(l1, l2);
    } else if (u1 <= Number::ZERO && u2 <= Number::ZERO) {
      lower_bound = Semantics::mul(u1, u2);
    } else {
      lower_bound = Number::INF;
    }
    // update upper bound
    if (l1 >= Number::ZERO && u2 <= Number::ZERO) {
      upper_bound = Semantics::mul(l1, u2);
    } else if (l2 >= Number::ZERO && u1 <= Number::ZERO) {
      upper_bound = Semantics::mul(l2, u1);
    } else if (u1 >= Number::ZERO && u2 >= Number::ZERO &&
               (l1 >= Number::ZERO || l2 >= Number::ZERO)) {
      upper_bound = Semantics::mul(u1, u2);
    } else {
      upper_bound = Number::INF;
    }
  }
  return *this;
}

Range& Range::operator/=(const Range& r) {
  const auto l1 = lower_bound;
  const auto l2 = r.lower_bound;
  const auto u1 = upper_bound;
  const auto u2 = r.upper_bound;
  if (r.isFinite()) {  // divisor is finite
    if (isFinite()) {  // dividend is also finite
      std::vector<Number> candidates;
      if (l2 <= Number::ZERO &&
          u2 >= Number::ZERO) {  // divisor range crosses zero
        candidates.push_back(u1);
        candidates.push_back(Semantics::mul(u1, Number::MINUS_ONE));
      } else {
        candidates = {Semantics::div(l1, l2), Semantics::div(l1, u2),
                      Semantics::div(u1, l2), Semantics::div(u1, u2)};
      }
      findMinMax(candidates.data(), candidates.size(), lower_bound,
                 upper_bound);
    } else {  // dividend is infinte, but divisor is finite
      // update lower bound
      if (l1 >= Number::ZERO && l2 >= Number::ZERO && u2 > Number::ZERO) {
        lower_bound = Semantics::div(l1, u2);
      } else if (u1 <= Number::ZERO && u2 <= Number::ZERO &&
                 l2 < Number::ZERO) {
        lower_bound = Semantics::div(l1, l2);
      } else {
        lower_bound = Number::INF;
      }
      // update upper bound
      if (u1 <= Number::ZERO && l2 >= Number::ZERO && u2 > Number::ZERO) {
        upper_bound = Semantics::div(u1, u2);
      } else if (l1 >= Number::ZERO && u2 <= Number::ZERO &&
                 l2 < Number::ZERO) {
        upper_bound = Semantics::div(u1, l2);
      } else {
        upper_bound = Number::INF;
      }
    }
  } else {  // divisor is infinite
    // update lower bound
    if ((l1 >= Number::ZERO && l2 >= Number::ZERO) ||
        (u1 <= Number::ZERO && u2 <= Number::ZERO)) {
      lower_bound = Number::ZERO;
    } else {
      lower_bound = Number::INF;
    }
    // update upper bound
    if ((l1 >= Number::ZERO && u2 <= Number::ZERO) ||
        (l2 >= Number::ZERO && u1 <= Number::ZERO)) {
      upper_bound = Number::ZERO;
    } else {
      upper_bound = Number::INF;
    }
  }
  return *this;
}

Range& Range::operator%=(const Range& r) {
  auto abs_lower = Semantics::abs(r.lower_bound);
  auto abs_upper = Semantics::abs(r.upper_bound);
  auto max_abs = Semantics::max(abs_lower, abs_upper);
  if (lower_bound >= Number::ZERO) {
    lower_bound = Number::ZERO;
    upper_bound = Semantics::sub(max_abs, Number::ONE);
  } else if (upper_bound <= Number::ZERO) {
    upper_bound = Number::ZERO;
    lower_bound = Semantics::sub(Number::ONE, max_abs);
  } else {
    lower_bound = Semantics::sub(Number::ONE, max_abs);
    upper_bound = Semantics::sub(max_abs, Number::ONE);
  }
  return *this;
}

void Range::gcd(const Range& r) {
  auto copy = *this;
  lower_bound = Number::ZERO;
  upper_bound = Number::INF;
  updateGcdBounds(copy);
  updateGcdBounds(r);
}

void Range::updateGcdBounds(const Range& r) {
  if (r.lower_bound > Number::ZERO) {
    lower_bound = Number::ONE;
    if (upper_bound == Number::INF || upper_bound > r.upper_bound) {
      upper_bound = r.upper_bound;
    }
  }
  if (r.upper_bound < Number::ZERO) {
    lower_bound = Number::ONE;
    auto abs = Semantics::abs(r.lower_bound);
    if (upper_bound == Number::INF || upper_bound > abs) {
      upper_bound = abs;
    }
  }
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

std::string RangeMap::toString(int64_t index, std::string name) const {
  if (name.empty()) {
    name = "$" + std::to_string(index);
  }
  auto it = find(index);
  std::string result;
  if (it != end()) {
    auto& r = it->second;  // FIX: use it->second, not it.second
    if (!r.isUnbounded()) {
      if (r.isConstant()) {
        result += name + " = " + r.lower_bound.to_string();
      } else if (r.lower_bound == Number::INF) {
        result += name + " <= " + r.upper_bound.to_string();
      } else if (r.upper_bound == Number::INF) {
        result += name + " >= " + r.lower_bound.to_string();
      } else {
        result += r.lower_bound.to_string() + " <= " + name +
                  " <= " + r.upper_bound.to_string();
      }
    }
  }
  return result;
}
