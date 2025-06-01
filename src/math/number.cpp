#include "math/number.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "math/big_number.hpp"

const Number Number::ZERO(0);
const Number Number::ONE(1);
const Number Number::TWO(2);
const Number Number::MINUS_ONE(-1);
const Number Number::MIN = Number::minMax(false);
const Number Number::MAX = Number::minMax(true);
const Number Number::INF = Number::infinity();

constexpr int64_t MIN_INT = std::numeric_limits<int64_t>::min();
constexpr int64_t MAX_INT = std::numeric_limits<int64_t>::max();
constexpr std::size_t MAX_SIZE = std::numeric_limits<std::size_t>::max();

BigNumber* INF_PTR = reinterpret_cast<BigNumber*>(1);

Number::Number()
    : value(0), big(FORCE_BIG_NUMBER ? new BigNumber(0) : nullptr) {}

Number::Number(const Number& n)
    : value(n.value),
      big((n.big && n.big != INF_PTR) ? new BigNumber(*n.big) : n.big) {}

Number::Number(int64_t value)
    : value(FORCE_BIG_NUMBER ? 0 : value),
      big(FORCE_BIG_NUMBER ? new BigNumber(value) : nullptr) {}

Number::Number(const std::string& s) {
  if (s == "inf") {
    value = 0;
    big = INF_PTR;
    return;
  }
  bool is_big = FORCE_BIG_NUMBER;
  if (!is_big) {
    if (s.size() <= 18) {
      value = std::stoll(s);
      big = nullptr;
    } else if (USE_BIG_NUMBER) {
      is_big = true;
    } else {
      value = 0;
      big = INF_PTR;
    }
  }
  if (is_big) {
    value = 0;
    big = new BigNumber(s);
    checkInfBig();
  }
}

Number::~Number() {
  if (big && big != INF_PTR) {
    delete big;
  }
}

Number& Number::operator=(const Number& n) {
  if (this != &n) {
    value = n.value;
    if (big && big != INF_PTR) {
      delete big;
    }
    big = (n.big && n.big != INF_PTR) ? new BigNumber(*n.big) : n.big;
  }
  return *this;
}

bool Number::operator==(const Number& n) const {
  if (big == INF_PTR) {
    return (n.big == INF_PTR);
  }
  if (n.big == INF_PTR) {
    return false;
  }
  if (big) {
    if (n.big) {
      return (*big) == (*n.big);
    } else {
      return (*big) == BigNumber(n.value);
    }
  } else if (n.big) {
    return BigNumber(value) == (*n.big);
  } else {
    return (value == n.value);
  }
}

bool Number::operator!=(const Number& n) const { return !(*this == n); }

bool Number::operator<(const Number& n) const {
  if (n.big == INF_PTR) {
    return (big != INF_PTR);
  }
  if (big == INF_PTR) {
    return false;
  }
  if (big) {
    if (n.big) {
      return (*big) < (*n.big);
    } else {
      return (*big) < BigNumber(n.value);
    }
  }
  if (n.big) {
    return BigNumber(value) < (*n.big);
  } else {
    return (value < n.value);
  }
}

bool Number::operator>(const Number& n) const { return (n < *this); }

bool Number::operator<=(const Number& n) const {
  return (*this < n || *this == n);
}

bool Number::operator>=(const Number& n) const {
  return (n < *this || *this == n);
}

Number& Number::negate() {
  if (big == INF_PTR) {
    return *this;
  }
  if (big) {
    big->negate();
    checkInfBig();
  } else if (value == MIN_INT) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      big->negate();
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
  } else {
    value = -value;
  }
  return *this;
}

Number& Number::operator+=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // One of the operands big?
  if (big) {
    if (n.big) {
      (*big) += (*n.big);
      checkInfBig();
    } else {
      (*big) += BigNumber(n.value);
      checkInfBig();
    }
    return *this;
  }
  if (n.big) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      (*big) += (*n.big);
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
    return *this;
  }
  // None of the operands is big
  if ((value > 0 && n.value > MAX_INT - value) ||
      (value < 0 && n.value < MIN_INT - value)) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      // It could be that *this == n. In that case, we just converted n to big
      // as well!
      if (n.big) {
        (*big) += (*n.big);
      } else {
        (*big) += BigNumber(n.value);
      }
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
  } else {
    value += n.value;
  }
  return *this;
}

Number& Number::operator-=(const Number& n) {
  auto m = n;
  m.negate();
  *this += m;
  return *this;
}

Number& Number::operator*=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // One of the operands big?
  if (big) {
    if (n.big) {
      (*big) *= (*n.big);
      checkInfBig();
    } else {
      (*big) *= BigNumber(n.value);
      checkInfBig();
    }
    return *this;
  }
  if (n.big) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      (*big) *= (*n.big);
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
    return *this;
  }
  // None of the operands is big
  if (n.value != 0 && (MAX_INT / std::abs(n.value) < std::abs(value))) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      // It could be that *this == n. In that case, we just converted n to big
      // as well!
      if (n.big) {
        (*big) *= (*n.big);
      } else {
        (*big) *= BigNumber(n.value);
      }
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
  } else {
    value *= n.value;
  }
  return *this;
}

Number& Number::operator/=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // One of the operands big?
  if (big) {
    if (n.big) {
      (*big) /= (*n.big);
      checkInfBig();
    } else {
      (*big) /= BigNumber(n.value);
      checkInfBig();
    }
    return *this;
  }
  if (n.big) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      (*big) /= (*n.big);
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
    return *this;
  }
  // None of the operands is big
  if (n.value == 0) {
    value = 0;
    big = INF_PTR;
  } else if (value == MIN_INT) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      // It could be that *this == n. In that case, we just converted n to big
      // as well!
      if (n.big) {
        (*big) /= (*n.big);
      } else {
        (*big) /= BigNumber(n.value);
      }
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
  } else {
    value /= n.value;
  }
  return *this;
}

Number& Number::operator%=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // One of the operands big?
  if (big) {
    if (n.big) {
      (*big) %= (*n.big);
      checkInfBig();
    } else {
      (*big) %= BigNumber(n.value);
      checkInfBig();
    }
    return *this;
  }
  if (n.big) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      (*big) %= (*n.big);
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
    return *this;
  }
  // None of the operands is big
  if (n.value == 0) {
    value = 0;
    big = INF_PTR;
  } else if (value == MIN_INT) {
    if (USE_BIG_NUMBER) {
      convertToBig();
      // It could be that *this == n. In that case, we just converted n to big
      // as well!
      if (n.big) {
        (*big) %= (*n.big);
      } else {
        (*big) %= BigNumber(n.value);
      }
      checkInfBig();
    } else {
      value = 0;
      big = INF_PTR;
    }
  } else {
    value %= n.value;
  }
  return *this;
}

Number& Number::operator&=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // First check if one of the operands is big. If not, perform the operation
  // on the small numbers. No need to check for INF after the operation, as the
  // result always fits in a Number. No need to check for USE_BIG_NUMBER, as we
  // never get a big number if both operands are small.
  if (big) {
    (*big) &= (n.big ? (*n.big) : BigNumber(n.value));
  } else if (n.big) {
    convertToBig();
    (*big) &= (*n.big);
  } else {
    const int64_t sign = (value < 0 && n.value < 0) ? -1 : 1;
    value = sign * (std::abs(value) & std::abs(n.value));
  }
  return *this;
}

Number& Number::operator|=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // First check if one of the operands is big. If not, perform the operation
  // on the small numbers. No need to check for INF after the operation, as the
  // result always fits in a Number. No need to check for USE_BIG_NUMBER, as we
  // never get a big number if both operands are small.
  if (big) {
    (*big) |= (n.big ? (*n.big) : BigNumber(n.value));
  } else if (n.big) {
    convertToBig();
    (*big) |= (*n.big);
  } else {
    const int64_t sign = (value < 0 || n.value < 0) ? -1 : 1;
    value = sign * (std::abs(value) | std::abs(n.value));
  }
  return *this;
}

Number& Number::operator^=(const Number& n) {
  if (checkInfArgs(n)) {
    return *this;
  }
  // First check if one of the operands is big. If not, perform the operation
  // on the small numbers. No need to check for INF after the operation, as the
  // result always fits in a Number. No need to check for USE_BIG_NUMBER, as we
  // never get a big number if both operands are small.
  if (big) {
    (*big) ^= (n.big ? (*n.big) : BigNumber(n.value));
  } else if (n.big) {
    convertToBig();
    (*big) ^= (*n.big);
  } else {
    const int64_t sign = ((value < 0) == (n.value >= 0)) ? -1 : 1;
    value = sign * (std::abs(value) ^ std::abs(n.value));
  }
  return *this;
}

int64_t Number::asInt() const {
  if (big == INF_PTR) {
    throw std::runtime_error("Infinity error");
  }
  if (big) {
    return big->asInt();
  }
  return value;
}

int64_t Number::getNumUsedWords() const {
  if (big && big != INF_PTR) {
    return big->getNumUsedWords();
  }
  return 1;
}

bool Number::odd() const {
  if (big == INF_PTR) {
    return false;  // by convention
  }
  if (big) {
    return big->odd();
  }
  return (value & 1);
}

std::size_t Number::hash() const {
  if (big == INF_PTR) {
    return MAX_SIZE;  // must be the same as in BigNumber!
  }
  if (big) {
    return big->hash();
  } else {
    // we must use the same hash values as in BigNumber!
    return BigNumber(value).hash();
  }
}

std::ostream& operator<<(std::ostream& out, const Number& n) {
  if (n.big == INF_PTR) {
    out << "inf";
  } else if (n.big) {
    out << *n.big;
  } else {
    out << n.value;
  }
  return out;
}

std::string Number::to_string() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void throwParseError() { throw std::runtime_error("Error parsing number"); }

void Number::readIntString(std::istream& in, std::string& out) {
  out.clear();
  auto ch = in.peek();
  if (!std::isdigit(ch) && ch != '-') {
    throwParseError();
  }
  out += (char)ch;
  in.get();
  while (true) {
    ch = in.peek();
    if (!std::isdigit(ch)) {
      break;
    }
    out += (char)ch;
    in.get();
  }
  if (out[0] == '0' && out.size() > 1) {
    throwParseError();
  }
  if (out[0] == '-' && (out.size() == 1 || out[1] == '0')) {
    throwParseError();
  }
}

Number Number::infinity() {
  Number inf(0);
  inf.big = INF_PTR;
  return inf;
}

Number Number::minMax(bool is_max) {
  Number m;
  m.value = 0;
  m.big = new BigNumber(BigNumber::minMax(is_max));
  return m;
}

bool Number::checkInfArgs(const Number& n) {
  if (big == INF_PTR) {
    return true;
  }
  if (n.big == INF_PTR) {
    if (big) {
      delete big;
    }
    big = INF_PTR;
    return true;
  }
  return false;
}

void Number::checkInfBig() {
  if (big && big != INF_PTR && big->isInfinite()) {
    delete big;
    value = 0;
    big = INF_PTR;
  }
}

void Number::convertToBig() {
  if (big == INF_PTR) {
    big = new BigNumber();
    big->makeInfinite();
  } else if (!big) {
    big = new BigNumber(value);
  }
  value = 0;
}
