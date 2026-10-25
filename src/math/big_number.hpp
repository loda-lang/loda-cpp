#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>

// Big number implementation based on boost::multiprecision::cpp_int.
// Values can occupy up to NUM_WORDS * 64 bits; any result that exceeds this
// range becomes infinite (see makeInfinite).
class BigNumber {
 public:
  static constexpr size_t NUM_WORDS = 60;
  using BigInt =
      boost::multiprecision::number<boost::multiprecision::cpp_int_backend<
          NUM_WORDS * 64, NUM_WORDS * 64,
          boost::multiprecision::signed_magnitude,
          boost::multiprecision::checked, void>>;

  BigNumber();

  explicit BigNumber(int64_t value);

  explicit BigNumber(const std::string& s);

  bool operator==(const BigNumber& n) const;

  bool operator!=(const BigNumber& n) const;

  bool operator<(const BigNumber& n) const;

  BigNumber& negate();

  BigNumber& operator+=(const BigNumber& n);

  BigNumber& operator*=(const BigNumber& n);

  BigNumber& operator/=(const BigNumber& n);

  BigNumber& operator%=(const BigNumber& n);

  BigNumber& operator&=(const BigNumber& n);

  BigNumber& operator|=(const BigNumber& n);

  BigNumber& operator^=(const BigNumber& n);

  std::size_t hash() const;

  std::string toString() const;

  friend std::ostream& operator<<(std::ostream& out, const BigNumber& n);

  inline bool isInfinite() const { return is_infinite; }

  void makeInfinite();

  int64_t asInt() const;

  // Returns true if the value can be losslessly represented as int64_t.
  bool fitsInInt64() const;

  int64_t getNumUsedWords() const;

  bool odd() const;

  static BigNumber minMax(bool is_max);

 private:
  void load(const std::string& s);

  bool isZero() const;

  BigInt value;
  bool is_infinite;
};
