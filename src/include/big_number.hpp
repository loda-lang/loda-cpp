#pragma once

#include <stdlib.h>

#include <array>
#include <iostream>

class BigNumber {
 public:
  static constexpr size_t NUM_WORDS = 48;
  static constexpr size_t NUM_WORD_DIGITS = 18;
  static constexpr size_t NUM_DIGITS = NUM_WORDS * NUM_WORD_DIGITS;
  static constexpr int64_t WORD_BASE_ROOT = 1000000000;
  static constexpr int64_t WORD_BASE = WORD_BASE_ROOT * WORD_BASE_ROOT;

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
  void load(const std::string& s);

  bool isZero() const;

  void add(const BigNumber& n);

  void sub(const BigNumber& n);

  void mulShort(int64_t n);

  void shift(int64_t n);

  void div(const BigNumber& n);

  void divShort(const int64_t n);

  void divBig(const BigNumber& n);

  std::array<int64_t, NUM_WORDS> words;
  bool is_negative;  // we don't want to expose this
  bool is_infinite;
};
