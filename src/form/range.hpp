#pragma once

#include <map>
#include <string>

#include "math/number.hpp"
#include "math/sequence.hpp"

class Range {
 public:
  Range() {}
  Range(const Number& lower, const Number& upper)
      : lower_bound(lower), upper_bound(upper) {}

  Range& operator+=(const Range& r);
  Range& operator-=(const Range& r);
  Range& operator*=(const Range& r);
  Range& operator/=(const Range& r);
  Range& operator%=(const Range& r);

  void trn(const Range& r);
  void pow(const Range& r);
  void gcd(const Range& r);
  void min(const Range& r);
  void max(const Range& r);

  bool isFinite() const;
  bool isConstant() const;
  bool isUnbounded() const;

  int64_t check(const Sequence& seq) const;

  Number lower_bound;
  Number upper_bound;

 private:
  void updateGcdBounds(const Range& r);
};

class RangeMap : public std::map<int64_t, Range> {
 public:
  RangeMap() = default;

  void prune();

  std::string toString() const;
  std::string toString(int64_t index, std::string name = "") const;
};
