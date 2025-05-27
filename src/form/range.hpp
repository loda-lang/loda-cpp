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

  Number lower_bound;
  Number upper_bound;
};

class RangeSet : public std::map<int64_t, Range> {
 public:
  RangeSet() = default;

  void prune();

  std::string toString() const;
};
