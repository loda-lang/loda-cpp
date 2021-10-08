#pragma once

#include <stdlib.h>

#include <array>
#include <iostream>

class BigNumber {
 public:
  static constexpr size_t NUM_WORDS = 50;

  BigNumber();

  BigNumber(int64_t value);

  BigNumber(const std::string& s);

  bool operator==(const BigNumber& n) const;

  bool operator!=(const BigNumber& n) const;

  bool operator<(const BigNumber& n) const;

  BigNumber& negate();

  BigNumber& operator+=(const BigNumber& n);

  BigNumber& operator*=(const BigNumber& n);

  BigNumber& operator/=(const BigNumber& n);

  BigNumber& operator%=(const BigNumber& n);

  std::size_t hash() const;

  std::string toString() const;

  friend std::ostream& operator<<(std::ostream& out, const BigNumber& n);

  inline bool isInfinite() const { return is_infinite; }

  void makeInfinite();

  int64_t asInt() const;

  int64_t getNumUsedWords() const;

  bool odd() const;

  static BigNumber minMax(bool is_max);

 private:
  static constexpr uint64_t HIGH_BIT_MASK = 0xFFFFFFFF00000000ull;
  static constexpr uint64_t LOW_BIT_MASK = 0x00000000FFFFFFFFull;

  void load(const std::string& s);

  bool isZero() const;

  void add(const BigNumber& n);

  void sub(const BigNumber& n);

  void mulShort(uint64_t n);

  void shift(int64_t n);

  void div(const BigNumber& n);

  void divShort(const uint64_t n);

  void divBig(const BigNumber& n);

  std::array<uint64_t, NUM_WORDS> words;
  bool is_negative;  // we don't want to expose this
  bool is_infinite;
};
