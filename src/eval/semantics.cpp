#include "eval/semantics.hpp"

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
  if (base == Number::ONE) {
    return 1;  // 1^x is always 1
  }
  if (base == -1) {
    return exp.odd() ? -1 : 1;  // (-1)^x
  }
  if (base == Number::ZERO) {
    if (Number::ZERO < exp) {
      return 0;  // 0^(positive number)
    }
    if (exp == Number::ZERO) {
      return 1;  // 0^0
    }
    return Number::INF;  // 0^(negative number)
  }
  if (exp < Number::ZERO) {
    return 0;
  }
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
    aa = bb;
    bb = r;
  }
  return aa;
}

Number Semantics::lex(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF) {
    return Number::INF;
  }
  if (b == Number::ZERO || b == Number::ONE) {
    return Number::ZERO;
  }
  auto r = Number::ZERO;
  auto aa = abs(a);
  auto bb = abs(b);
  while (true) {
    auto aaa = dif(aa, bb);
    if (aaa == aa) {
      break;
    }
    aa = aaa;
    r += Number::ONE;
  }
  return r;
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

Number Semantics::log(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF || a < Number::ONE ||
      b < Number::TWO) {
    return Number::INF;
  }
  if (a == Number::ONE) {
    return Number::ZERO;
  }
  auto m = Number::ONE;
  auto res = Number::ZERO;
  while (m < a) {
    m = mul(m, b);
    res += Number::ONE;
  }
  return (m == a) ? res : sub(res, 1);
}

Number Semantics::nrt(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF || a < Number::ZERO ||
      b < Number::ONE) {
    return Number::INF;
  }
  if (a == Number::ZERO || a == Number::ONE || b == Number::ONE) {
    return a;
  }
  auto r = Number::ONE;
  auto l = Number::ZERO;
  auto h = a;
  while (l < h) {
    auto m = div(add(l, h), 2);
    auto p = pow(m, b);
    if (p == a) {
      return m;
    }
    if (p < a) {
      l = m;
    } else {
      h = m;
    }
    if (r == m) {
      break;
    }
    r = m;
  }
  return r;
}

Number Semantics::dgs(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF || b < Number::TWO) {
    return Number::INF;
  }
  const int64_t sign = a < Number::ZERO ? -1 : 1;
  auto aa = abs(a);
  auto r = Number::ZERO;
  while (aa > Number::ZERO && r != Number::INF && aa != Number::INF) {
    r += mod(aa, b);
    aa /= b;
  }
  return mul(sign, r);
}

Number Semantics::dgr(const Number& a, const Number& b) {
  if (a == Number::INF || b == Number::INF || b < Number::TWO) {
    return Number::INF;
  }
  if (a == Number::ZERO) {
    return Number::ZERO;
  }
  return mul(
      a < Number::ZERO ? Number::MINUS_ONE : Number::ONE,  // sign
      add(Number::ONE, mod(sub(abs(a), Number::ONE), sub(b, Number::ONE))));
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

Number Semantics::ban(const Number& a, const Number& b) {
  auto r = a;
  r &= b;
  return r;
}

Number Semantics::bor(const Number& a, const Number& b) {
  auto r = a;
  r |= b;
  return r;
}

Number Semantics::bxo(const Number& a, const Number& b) {
  auto r = a;
  r ^= b;
  return r;
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
  if (value < Number::ONE || base < Number::TWO) {
    return Number::INF;
  }
  int64_t result = 0;
  while (mod(value, base) == Number::ZERO) {
    result++;
    value = div(value, base);
  }
  return (value == Number::ONE) ? result : 0;
}
