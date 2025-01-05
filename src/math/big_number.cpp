#include "math/big_number.hpp"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

BigNumber::BigNumber() : is_negative(false), is_infinite(false) {
  words.fill(0);
}

BigNumber::BigNumber(int64_t value) {
  if (value >= 0) {
    is_negative = false;
    is_infinite = false;
    words.fill(0);
    words[0] = value;
  } else {
    load(std::to_string(value));
  }
}

BigNumber::BigNumber(const std::string &s) { load(s); }

void throwNumberParseError(const std::string &s) {
  throw std::invalid_argument("error reading number: '" + s + "'");
}

void BigNumber::load(const std::string &s) {
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
  if (s[start] == '-') {
    is_negative = true;
    if (++start == size) {
      throwNumberParseError(s);
    }
  } else {
    is_negative = false;
  }
  size -= start;
  while (size > 0 && s[start + size - 1] == ' ') {
    size--;
  }
  if (size == 0) {
    throwNumberParseError(s);
  }
  words.fill(0);
  for (int64_t i = 0; i < size; i++) {
    if (is_infinite) {
      break;
    }
    char ch = s[start + i];
    if (ch < '0' || ch > '9') {
      throwNumberParseError(s);
    }
    mulShort(10);
    add(BigNumber(ch - '0'));  // TODO: use addShort
  }
}

bool BigNumber::isZero() const {
  return !is_infinite && std::all_of(words.begin(), words.end(),
                                     [](uint64_t w) { return w == 0; });
}

void BigNumber::makeInfinite() {
  is_negative = false;
  is_infinite = true;
  words.fill(0);
}

int64_t BigNumber::asInt() const {
  if (is_infinite) {
    throw std::runtime_error("Infinity error");
  }
  if (words[0] > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
    throw std::runtime_error("Integer overflow");
  }
  for (size_t i = 1; i < NUM_WORDS; i++) {
    if (words[i] != 0) {
      throw std::runtime_error("Integer overflow");
    }
  }
  return is_negative ? -words[0] : words[0];
}

int64_t BigNumber::getNumUsedWords() const {
  if (is_infinite) {
    return 1;
  }
  for (int64_t i = NUM_WORDS - 1; i >= 0; i--) {
    if (words[i] != 0) {
      return i + 1;
    }
  }
  return 1;
}

bool BigNumber::odd() const {
  if (is_infinite) {
    return false;  // by convention
  }
  return (words[0] & 1);
}

BigNumber BigNumber::minMax(bool is_max) {
  BigNumber m;
  m.is_infinite = false;
  m.is_negative = !is_max;
  m.words.fill(std::numeric_limits<uint64_t>::max());
  return m;
}

bool BigNumber::operator==(const BigNumber &n) const {
  if (is_infinite != n.is_infinite) {
    return false;
  }
  if (words != n.words) {
    return false;
  }
  return ((is_negative == n.is_negative) || isZero());
}

bool BigNumber::operator!=(const BigNumber &n) const { return !(*this == n); }

bool BigNumber::operator<(const BigNumber &n) const {
  bool is_zero = true;
  for (int64_t i = NUM_WORDS - 1; i >= 0; i--) {
    if (words[i] < n.words[i]) {
      return !n.is_negative;
    } else if (words[i] > n.words[i]) {
      return is_negative;
    }
    is_zero = is_zero && (words[i] == 0);
  }
  return !is_zero && is_negative && !n.is_negative;
}

BigNumber &BigNumber::negate() {
  // note that this can lead to -0 (therefore we don't expose is_negative)
  is_negative = !is_negative;
  return *this;
}

BigNumber &BigNumber::operator+=(const BigNumber &n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  // check if one of the operands is negative
  if (!is_negative && n.is_negative) {
    BigNumber m(n);
    m.is_negative = false;
    if (*this < m) {
      m.sub(*this);
      (*this) = m;
      is_negative = true;
    } else {
      sub(m);
    }
  } else if (is_negative && !n.is_negative) {
    BigNumber m(n);
    is_negative = false;
    if (*this < m) {
      m.sub(*this);
      (*this) = m;
    } else {
      sub(m);
      is_negative = true;
    }
  } else {
    add(n);
  }
  return *this;
}

void BigNumber::add(const BigNumber &n) {
  uint64_t carry = 0;
  for (size_t i = 0; i < NUM_WORDS; i++) {
    uint64_t low, high;
    low = (words[i] & LOW_BIT_MASK) + (n.words[i] & LOW_BIT_MASK) + carry;
    carry = low >> 32;
    high = (words[i] >> 32) + (n.words[i] >> 32) + carry;
    carry = high >> 32;
    words[i] = ((high & LOW_BIT_MASK) << 32) | (low & LOW_BIT_MASK);
  }
  if (carry) {
    makeInfinite();
  }
}

void BigNumber::sub(const BigNumber &n) {
  uint64_t carry = 0;
  for (size_t i = 0; i < NUM_WORDS; i++) {
    uint64_t low, high;
    low = (words[i] & LOW_BIT_MASK) - (n.words[i] & LOW_BIT_MASK) - carry;
    carry = (low >> 32) != 0;
    high = (words[i] >> 32) - (n.words[i] >> 32) - carry;
    carry = (high >> 32) != 0;
    words[i] = ((high & LOW_BIT_MASK) << 32) | (low & LOW_BIT_MASK);
  }
  if (carry) {
    is_negative = true;
  }
}

BigNumber &BigNumber::operator*=(const BigNumber &n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  BigNumber result(0);
  int64_t shift = 0;
  const int64_t s = n.getNumUsedWords();  // <= NUM_WORDS
  for (int64_t i = 0; i < s; i++) {
    auto copy = *this;
    copy.mulShort(n.words[i] & LOW_BIT_MASK);  // low bits
    copy.shift(shift);
    shift += 1;
    result += copy;
    copy = *this;
    copy.mulShort(n.words[i] >> 32);  // high bits
    copy.shift(shift);
    shift += 1;
    result += copy;
    if (result.is_infinite) {
      break;
    }
  }
  if (!result.is_infinite) {
    result.is_negative = (is_negative != n.is_negative);
  }
  (*this) = result;
  return (*this);
}

void BigNumber::mulShort(uint64_t n) {
  uint64_t carry = 0;
  const int64_t s = std::min<int64_t>(getNumUsedWords() + 1, NUM_WORDS);
  for (int64_t i = 0; i < s; i++) {
    uint64_t low, high;
    auto &w = words[i];
    high = (w >> 32) * n;
    low = (w & LOW_BIT_MASK) * n;
    w = low + ((high & LOW_BIT_MASK) << 32) + carry;
    carry = ((high + ((low + carry) >> 32)) >> 32);
  }
  if (carry) {
    makeInfinite();
  }
}

void BigNumber::shift(int64_t n) {
  for (; n > 0; n--) {
    uint64_t next = 0;
    for (size_t i = 0; i < NUM_WORDS; i++) {
      uint64_t h = words[i] >> 32;
      uint64_t l = words[i] & LOW_BIT_MASK;
      words[i] = (l << 32) + next;
      next = h;
    }
    if (next) {
      makeInfinite();
      break;
    }
  }
}

BigNumber &BigNumber::operator/=(const BigNumber &n) {
  if (is_infinite || n.is_infinite || n.isZero()) {
    makeInfinite();
    return *this;
  }
  auto m = n;  // TODO: avoid this copy is possible
  bool new_is_negative = (m.is_negative != is_negative);
  m.is_negative = false;
  is_negative = false;
  div(m);
  is_negative = new_is_negative;
  return *this;
}

void BigNumber::div(const BigNumber &n) {
  if (n.getNumUsedWords() == 1 && !(n.words[0] >> 32)) {
    divShort(n.words[0]);
  } else {
    divBig(n);
  }
}

void BigNumber::divShort(const uint64_t n) {
  uint64_t carry = 0;
  for (int64_t i = NUM_WORDS - 1; i >= 0; i--) {
    uint64_t h, l, t, h2, u, l2;
    auto &w = words[i];
    h = w >> 32;
    l = w & LOW_BIT_MASK;
    t = (carry << 32) + h;
    h2 = t / n;
    carry = t % n;
    u = (carry << 32) + l;
    l2 = u / n;
    carry = u % n;
    w = (h2 << 32) + l2;
  }
}

void BigNumber::divBig(const BigNumber &n) {
  std::vector<std::pair<BigNumber, BigNumber>> d;
  BigNumber f(n);
  BigNumber g(1);
  while (f < *this || f == *this) {
    d.emplace_back(f, g);
    f += f;
    g += g;
    if (f.is_infinite || g.is_infinite) {
      makeInfinite();
      return;
    }
  }
  BigNumber r(0);
  for (auto it = d.rbegin(); it != d.rend(); it++) {
    while (it->first < *this || it->first == *this) {
      sub(it->first);
      r.add(it->second);
      if (r.is_infinite) {
        break;
      }
    }
  }
  *this = r;
}

BigNumber &BigNumber::operator%=(const BigNumber &n) {
  if (is_infinite || n.is_infinite || n.isZero()) {
    makeInfinite();
    return *this;
  }
  auto m = n;
  const bool new_is_negative = is_negative;
  m.is_negative = false;
  is_negative = false;
  auto q = *this;
  q.div(m);
  if (q.is_infinite) {
    makeInfinite();
    return *this;
  }
  q *= m;
  if (q.is_infinite) {
    makeInfinite();
    return *this;
  }
  sub(q);
  is_negative = new_is_negative;
  return *this;
}

BigNumber &BigNumber::operator&=(const BigNumber &n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  for (size_t i = 0; i < NUM_WORDS; i++) {
    words[i] &= n.words[i];
  }
  is_negative = is_negative && n.is_negative;
  return (*this);
}

BigNumber &BigNumber::operator|=(const BigNumber &n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  for (size_t i = 0; i < NUM_WORDS; i++) {
    words[i] |= n.words[i];
  }
  is_negative = is_negative || n.is_negative;
  return (*this);
}

BigNumber &BigNumber::operator^=(const BigNumber &n) {
  if (is_infinite || n.is_infinite) {
    makeInfinite();
    return *this;
  }
  for (size_t i = 0; i < NUM_WORDS; i++) {
    words[i] ^= n.words[i];
  }
  is_negative = is_negative != n.is_negative;
  return (*this);
}

std::size_t BigNumber::hash() const {
  if (is_infinite) {
    return std::numeric_limits<std::size_t>::max();
  }
  std::size_t seed = 0;
  bool is_zero = true;
  for (const auto &w : words) {
    seed ^= w + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    is_zero = is_zero && (w != 0);
  }
  if (!is_zero && is_negative) {
    seed ^= 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

std::string BigNumber::toString() const {
  if (is_infinite) {
    return "inf";
  }
  if (isZero()) {
    return "0";
  }
  std::string result;
  BigNumber m = *this;
  BigNumber n;
  while (!m.isZero()) {
    n = m;
    n %= BigNumber(10);  // avoid this expensive mod operation
    result += ('0' + n.words[0]);
    m.divShort(10);
  }
  if (is_negative) {
    result += '-';
  }
  std::reverse(result.begin(), result.end());
  return result;
}

std::ostream &operator<<(std::ostream &out, const BigNumber &n) {
  out << n.toString();
  return out;
}
