#pragma once

#include <unordered_map>

#include "number.hpp"

struct number_pair_hasher {
  std::size_t operator()(const std::pair<Number, Number>& p) const {
    return (p.first.hash() << 32) ^ p.second.hash();
  }
};

class Semantics {
 public:
  static Number add(const Number& a, const Number& b);

  static Number sub(const Number& a, const Number& b);

  static Number trn(const Number& a, const Number& b);

  static Number mul(const Number& a, const Number& b);

  static Number div(const Number& a, const Number& b);

  static Number dif(const Number& a, const Number& b);

  static Number mod(const Number& a, const Number& b);

  static Number pow(const Number& base, const Number& exp);

  static Number gcd(const Number& a, const Number& b);

  static Number bin(const Number& n, const Number& k);

  static Number cmp(const Number& a, const Number& b);

  static Number min(const Number& a, const Number& b);

  static Number max(const Number& a, const Number& b);

  static Number abs(const Number& a);

  static Number getPowerOf(Number value, const Number& base);

 private:
  static bool HAS_MEMORY;
  static size_t NUM_MEMORY_CHECKS;
  static std::unordered_map<std::pair<Number, Number>, Number,
                            number_pair_hasher>
      BIN_CACHE;
};
