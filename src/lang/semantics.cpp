#include "lang/semantics.hpp"

#include "lang/number.hpp"

Number Semantics::add(const Number& a, const Number& b) {
  auto r = a;
  r += b;
  return r;
}

Number Semantics::sub(const Number& a, const Number& b) {
  auto r = a;
  r -= b;
  return r;
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
      Number r = 1;
      Number b = base;
      Number e = exp;
      while (r != Number::INF && e != Number::ZERO) {
        if (e.odd()) {
          r = mul(r, b);
        }
        e = div(e, 2);
        if (e != Number::ZERO) {
          b = mul(b, b);
          if (b == Number::INF) {
            r = Number::INF;
          }
        }
      }
      return r;
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

  // check argument size
  if (k.getNumUsedWords() > 1) {
    return Number::INF;
  }
  auto l = k.asInt();

  // main computation
  Number r(1);
  for (int64_t i = 0; i < l; i++) {
    r = mul(r, sub(n, i));
    r = div(r, add(i, 1));
    if (r == Number::INF) {
      break;
    }
  }
  return mul(sign, r);
}

Number Semantics::equ(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a == b) ? 1 : 0;
}

Number Semantics::neq(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a != b) ? 1 : 0;
}

Number Semantics::leq(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (a < b || a == b) ? 1 : 0;
}

Number Semantics::geq(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  return (b < a || a == b) ? 1 : 0;
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
  if (value == Number::INF || base == Number::INF) {
    return Number::INF;
  }
  if (value < Number::ONE || base < Number(2)) {
    return Number::INF;
  }
  int64_t result = 0;
  while (mod(value, base) == Number::ZERO) {
    result++;
    value = div(value, base);
  }
  return (value == Number::ONE) ? result : 0;
}
