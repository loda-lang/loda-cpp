#pragma once

#include <map>
#include <string>

#include "math/number.hpp"

class Range {
 public:
  Range() {}
  Range(const Number& lower, const Number& upper)
      : lower_bound(lower), upper_bound(upper) {}

  Range& operator+=(const Range& r);
  Range& operator-=(const Range& r);
  Range& operator%=(const Range& r);

  bool isFinite() const;
  bool isConstant() const;
  bool isUnbounded() const;

  Number lower_bound;
  Number upper_bound;
};

class RangeMap : public std::map<int64_t, Range> {
 public:
  RangeMap() = default;

  void prune();

  std::string toString() const;
};
