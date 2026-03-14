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
  void dif(const Range& r);
  void dir(const Range& r);
  void pow(const Range& r);
  void gcd(const Range& r);
  void lex(const Range& r);
  void bin(const Range& r);
  void fac(const Range& r);
  void log(const Range& r);
  void nrt(const Range& r);
  void dgs(const Range& r);
  void dgr(const Range& r);
  void min(const Range& r);
  void max(const Range& r);
  void binary(const Range& r);

  bool isFinite() const;
  bool isConstant() const;
  bool isUnbounded() const;

  bool check(const Number& n) const;
  int64_t check(const Sequence& seq) const;

  std::string toString(const std::string& name) const;

  Number lower_bound;
  Number upper_bound;

 private:
  void updateGcdBounds(const Range& r);
};

class RangeMap : public std::map<int64_t, Range> {
 public:
  RangeMap() = default;

  Range get(int64_t index) const;
  void prune();

  std::string toString() const;
  std::string toString(int64_t index, std::string name = "") const;
};
