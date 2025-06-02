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
  
  if (lower_bound != Number::INF && upper_bound != Number::INF && r.lower_bound != Number::INF && r.upper_bound != Number::INF){
auto a1 = Semantics::mul(lower_bound, r.lower_bound);
auto a2 = Semantics::mul(lower_bound, r.upper_bound);
auto a3 = Semantics::mul(upper_bound, r.lower_bound);
auto a4 = Semantics::mul(upper_bound, r.upper_bound);
l = a1;u = a1;
if(l > a2)l = a2;if(u < a2)u = a2;
if(l > a3)l = a3;if(u < a3)u = a3;
if(l > a4)l = a4;if(u < a4)u = a4;
  lower_bound = l;
  upper_bound = u;
  return *this;
  }
  
  if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
      r.lower_bound != Number::INF && r.lower_bound >= Number::ZERO) {
    l = Semantics::mul(lower_bound, r.lower_bound);
  }
  if (lower_bound != Number::INF && lower_bound >= Number::ZERO &&
      r.lower_bound != Number::INF && r.lower_bound <= Number::ZERO &&
      r.upper_bound != Number::INF) {
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

Range& Range::concat(const Range& r) {
  lower_bound = Semantics::min(lower_bound, r.lower_bound);
  upper_bound = Semantics::max(upper_bound, r.upper_bound);
  return *this;
}

Range& Range::gcd(const Range& r) {
  auto l = Number::ZERO;
  auto u = Number::INF;
  
  if (lower_bound != Number::INF && lower_bound >= Number::ONE &&
      r.lower_bound != Number::INF && r.lower_bound >= Number::ONE) {
    l = Number::ONE;
  }
  if (lower_bound != Number::INF && lower_bound >= Number::ONE) {
    if(u == Number::INF)u = upper_bound;
    else if(u > upper_bound)u = upper_bound;
  }
  if (r.lower_bound != Number::INF && r.lower_bound >= Number::ONE) {
    if(u == Number::INF)u = r.upper_bound;
    else if(u > r.upper_bound)u = r.upper_bound;
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
