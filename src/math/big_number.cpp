#include "math/big_number.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

void throwNumberParseError(const std::string& s) {
  throw std::invalid_argument("error reading number: '" + s + "'");
}

// Maximum magnitude: 2^(NUM_WORDS * 64) - 1. Larger results become infinite.
// Function-local static to be independent of static initialization order.
// Built word by word: the intermediate 2^(NUM_WORDS * 64) itself does not fit
// into the fixed precision backend.
static const BigNumber::BigInt& maxMagnitude() {
  static const BigNumber::BigInt m = [] {
    BigNumber::BigInt mag = 0;
    for (size_t i = 0; i < BigNumber::NUM_WORDS; i++) {
      mag <<= 64;                    // magic number: 64 bits per word
      mag += 0xFFFFFFFFFFFFFFFFull;  // magic number: all bits set
    }
    return mag;
  }();
  return m;
}

BigNumber::BigNumber() : value(0), is_infinite(false) {}

BigNumber::BigNumber(int64_t value) : value(value), is_infinite(false) {}

BigNumber::BigNumber(const std::string& s) { load(s); }

void BigNumber::load(const std::string& s) {
  if (s == "inf") {
    makeInfinite();
    return;
  }
  is_infinite = false;
  int64_t size = s.length();
  int64_t start = 0;
  while (start < size && s[start] == ' ') {
    start++;
  }
  if (start == size) {
    throwNumberParseError(s);
  }
  const bool is_negative = (s[start] == '-');
  if (is_negative) {
    if (++start == size) {
      throwNumberParseError(s);
    }
  }
  size -= start;
  while (size > 0 && s[start + size - 1] == ' ') {
    size--;
  }
  if (size == 0) {
    throwNumberParseError(s);
  }
  for (int64_t i = 0; i < size; i++) {
    char ch = s[start + i];
    if (ch < '0' || ch > '9') {
      throwNumberParseError(s);
    }
  }
  // Strip leading zeros: boost would otherwise interpret them as an octal
  // radix (or reject digits >= 8 in octal mode).
  int64_t skip = 0;
  while (skip + 1 < size && s[start + skip] == '0') {
    skip++;
  }
  try {
    value = BigInt(s.substr(start + skip, size - skip));
    if (is_negative) {
      value = -value;
    }
  } catch (const std::overflow_error&) {
    makeInfinite();  // number too large for the available range
  }
}

bool BigNumber::isZero() const { return !is_infinite && (value == 0); }

void BigNumber::makeInfinite() {
  is_infinite = true;
  value = 0;
}

int64_t BigNumber::asInt() const {
  if (is_infinite) {
    throw std::runtime_error("Infinity error");
  }
  if (!fitsInInt64()) {
    throw std::runtime_error("Integer overflow");
  }
  return value.convert_to<int64_t>();
}

bool BigNumber::fitsInInt64() const {
  if (is_infinite) {
    return false;
  }
  static const BigInt MAX_INT(
      (std::numeric_limits<int64_t>::max)());  // magic number
  static const BigInt MIN_INT(
      (std::numeric_limits<int64_t>::min)());  // magic number
  return (value <= MAX_INT) && (value >= MIN_INT);
}

int64_t BigNumber::getNumUsedWords() const {
  if (is_infinite || isZero()) {
    return 1;
  }
  const int64_t bits = boost::multiprecision::msb(abs(value)) + 1;
  return (bits + 63) / 64;  // magic number: 64 bits per word
}

bool BigNumber::odd() const {
  if (is_infinite) {
    return false;  // by convention
  }
  return bit_test(value, 0);
}

BigNumber BigNumber::minMax(bool is_max) {
  BigNumber m;
  m.is_infinite = false;
  m.value = maxMagnitude();
  if (!is_max) {
    m.value = -m.value;
  }
  return m;
}

bool BigNumber::operator==(const BigNumber& n) const {
  if (is_infinite != n.is_infinite) {
    return false;
  }
  if (is_infinite) {
    return true;
  }
  return (value == n.value);
}

bool BigNumber::operator!=(const BigNumber& n) const { return !(*this == n); }

bool BigNumber::operator<(const BigNumber& n) const {
  if (is_infinite || n.is_infinite) {
    return false;
  }
  return (value < n.value);
}

BigNumber& BigNumber::negate() {
  if (!is_infinite) {
    value = -value;
  }
  return *this;
}

BigNumber& BigNumber::operator+=(const BigNumber& n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  try {
    value += n.value;
  } catch (const std::overflow_error&) {
    makeInfinite();
  }
  return *this;
}

BigNumber& BigNumber::operator*=(const BigNumber& n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  try {
    value *= n.value;
  } catch (const std::overflow_error&) {
    makeInfinite();
  }
  return *this;
}

BigNumber& BigNumber::operator/=(const BigNumber& n) {
  if (is_infinite || n.is_infinite || n.isZero()) {
    makeInfinite();
    return *this;
  }
  value /= n.value;  // truncates towards zero
  return *this;
}

BigNumber& BigNumber::operator%=(const BigNumber& n) {
  if (is_infinite || n.is_infinite || n.isZero()) {
    makeInfinite();
    return *this;
  }
  value %= n.value;  // sign of the result follows the first operand
  return *this;
}

// The checked backend rejects bitwise operations on negative values, so we
// perform them via the two's complement representation over the full
// NUM_WORDS * 64 bits (the checked backend rejects bitwise operations on
// negative values directly).
static const boost::multiprecision::cpp_int& twoPowerMaxBits() {
  static const boost::multiprecision::cpp_int p =
      boost::multiprecision::cpp_int(1) << (BigNumber::NUM_WORDS * 64);
  return p;
}

static boost::multiprecision::cpp_int toTwosComplement(
    const BigNumber::BigInt& v) {
  boost::multiprecision::cpp_int result(v);
  if (v < 0) {
    result += twoPowerMaxBits();
  }
  return result;
}

static BigNumber::BigInt fromTwosComplement(
    const boost::multiprecision::cpp_int& v) {
  if (v < (twoPowerMaxBits() >> 1)) {
    return BigNumber::BigInt(v);
  }
  return BigNumber::BigInt(v - twoPowerMaxBits());  // negative value
}

BigNumber& BigNumber::operator^=(const BigNumber& n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  auto a = toTwosComplement(value);
  a ^= toTwosComplement(n.value);
  value = fromTwosComplement(a);
  return *this;
}

BigNumber& BigNumber::operator&=(const BigNumber& n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  auto a = toTwosComplement(value);
  a &= toTwosComplement(n.value);
  value = fromTwosComplement(a);
  return *this;
}

BigNumber& BigNumber::operator|=(const BigNumber& n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  auto a = toTwosComplement(value);
  a |= toTwosComplement(n.value);
  value = fromTwosComplement(a);
  return *this;
}

std::size_t BigNumber::hash() const {
  if (is_infinite) {
    return std::numeric_limits<std::size_t>::max();
  }
  std::size_t seed = 0;
  const bool is_zero = isZero();
  if (!is_zero) {
    static const BigInt MASK((static_cast<uint64_t>(-1)));  // magic number
    BigInt tmp = abs(value);
    while (tmp != 0) {
      const uint64_t chunk = static_cast<uint64_t>(tmp & MASK);
      seed ^= chunk + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      tmp >>= 64;
    }
    if (value < 0) {
      seed ^= 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
  }
  return seed;
}

std::string BigNumber::toString() const {
  if (is_infinite) {
    return "inf";
  }
  return value.str();
}

std::ostream& operator<<(std::ostream& out, const BigNumber& n) {
  out << n.toString();
  return out;
}
