#include "semantics.hpp"

#include "number.hpp"
#include "setup.hpp"

bool Semantics::HAS_MEMORY = true;
size_t Semantics::NUM_MEMORY_CHECKS = 0;
std::unordered_map<std::pair<Number, Number>, Number, number_pair_hasher>
    Semantics::BIN_CACHE;

Number Semantics::add(const Number& a, const Number& b) {
  auto r = a;
  r += b;
  return r;
}

Number Semantics::sub(const Number& a, const Number& b) {
  auto c = b;
  c.negate();
  return add(a, c);
}

Number Semantics::trn(const Number& a, const Number& b) {
  return max(sub(a, b), Number::ZERO);
}

Number Semantics::mul(const Number& a, const Number& b) {
  auto r = a;
  r *= b;
  return r;
}

Number Semantics::div(const Number& a, const Number& b) {
  auto r = a;
  r /= b;
  return r;
}

Number Semantics::dif(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  if (b == Number::ZERO) {
    return a;
  }
  const auto d = div(a, b);
  return (a == mul(b, d)) ? d : a;
}

Number Semantics::mod(const Number& a, const Number& b) {
  auto r = a;
  r %= b;
  return r;
}

Number Semantics::pow(const Number& base, const Number& exp) {
  if (base == Number::INF || exp == Number::INF) {
    return Number::INF;
  }
  if (base == Number::ZERO) {
    if (Number::ZERO < exp) {
      return 0;  // 0^(positive number)
    } else if (exp == Number::ZERO) {
      return 1;  // 0^0
    } else {
      return Number::INF;  // 0^(negative number)
    }
  } else if (base == Number::ONE) {
    return 1;  // 1^x is always 1
  } else if (base == -1) {
    return exp.odd() ? -1 : 1;  // (-1)^x
  } else {
    if (exp < Number::ZERO) {
      return 0;
    } else {
      Number res = 1;
      for (int64_t e = exp.asInt(); e > 0; e--) {
        res *= base;
        if (res == Number::INF) {
          break;
        }
      }
      // TODO: fix fast pow
      /*
      Number b = base;
      Number e = exp;
      while (res != Number::INF && Number::ZERO < e) {
        if (e.odd()) {
          res = mul(res, b);
        }
        e = div(e, 2);
        b = mul(b, b);
        if (b == Number::INF) {
          res = Number::INF;
        }
      }
      */
      return res;
    }
  }
}

Number Semantics::gcd(const Number& a, const Number& b) {
  if (a == Number::ZERO && b == Number::ZERO) {
    return Number::ZERO;
  }
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  auto aa = abs(a);
  auto bb = abs(b);
  Number r;
  while (bb != Number::ZERO) {
    r = mod(aa, bb);
    if (r == Number::INF) {
      return Number::INF;
    }
    aa = bb;
    bb = r;
  }
  return aa;
}

Number Semantics::bin(const Number& nn, const Number& kk) {
  if (nn == Number::INF || kk == Number::INF) {
    return Number::INF;
  }
  auto n = nn;
  auto k = kk;

  // check for negative arguments: https://arxiv.org/pdf/1105.3689.pdf
  Number sign(1);
  if (n < Number::ZERO)  // Theorem 2.1
  {
    if (!(k < Number::ZERO)) {
      sign = k.odd() ? -1 : 1;
      n = sub(k, add(n, Number::ONE));
    } else if (!(n < k)) {
      sign = sub(n, k).odd() ? -1 : 1;
      auto n_old = n;
      n = sub(Number::ZERO, add(k, Number::ONE));
      k = sub(n_old, k);
    } else {
      return 0;
    }
  }
  if (k < Number::ZERO || n < k)  // 1.2
  {
    return 0;
  }
  if (n < mul(k, 2)) {
    k = sub(n, k);
  }
  if (k.getNumUsedWords() > 1) {
    return Number::INF;
  }

  // check if the value is cached already
  const std::pair<Number, Number> key(n, k);
  auto it = BIN_CACHE.find(key);
  if (it != BIN_CACHE.end()) {
    return it->second;
  }

  // main computation
  Number r(1);
  auto l = k.asInt();
  for (int64_t i = 0; i < l; i++) {
    r = mul(r, sub(n, i));
    r = div(r, add(i, 1));
    if (r == Number::INF) {
      break;
    }
  }
  r = mul(sign, r);

  // add to cache if there is memory available
  if (++NUM_MEMORY_CHECKS % 10000 == 0) {  // magic number
    HAS_MEMORY = Setup::hasMemory();
  }
  if (HAS_MEMORY || BIN_CACHE.size() < 10000) {  // magic number
    BIN_CACHE[key] = r;
  }
  return r;
}

Number Semantics::cmp(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a == b) ? 1 : 0;
}

Number Semantics::min(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a < b) ? a : b;
}

Number Semantics::max(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a < b) ? b : a;
}

Number Semantics::abs(const Number& a) {
  if (a == Number::INF) {
    return Number::INF;
  }
  return (a < Number::ZERO) ? mul(a, -1) : a;
}

Number Semantics::getPowerOf(Number value, const Number& base) {
  if (value == Number::INF || base == Number::INF || base == Number::ZERO) {
    return Number::INF;
  }
  int64_t result = 0;
  while (mod(value, base) == Number::ZERO) {
    result++;
    value = div(value, base);
  }
  return (value == Number::ONE) ? result : 0;
}
