#pragma once

#include <array>
#include <iostream>
#include <string>
#include <unordered_map>

#include "math/number.hpp"

#define MEMORY_CACHE_SIZE 16

class Memory {
 public:
  Memory();

  Memory(const std::string &s);

  Number get(int64_t index) const;

  void set(int64_t index, const Number &value);

  void clear();

  void clear(int64_t start, int64_t length);

  Memory fragment(int64_t start, int64_t length) const;

  size_t approximate_size() const;

  bool is_less(const Memory &m, int64_t length, bool check_nonn) const;

  bool operator==(const Memory &m) const;

  bool operator!=(const Memory &m) const;

  friend std::ostream &operator<<(std::ostream &out, const Memory &m);

 private:
  std::array<Number, MEMORY_CACHE_SIZE> cache;
  std::unordered_map<int64_t, Number> full;
};
