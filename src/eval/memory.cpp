#include "eval/memory.hpp"

#include <map>
#include <stdexcept>
#include <string>

Memory::Memory() { cache.fill(0); }

Memory::Memory(const std::string &s) {
  cache.fill(0);
  size_t pos = 0;
  while (pos < s.size()) {
    size_t next = s.find(',', pos);
    if (next == std::string::npos) {
      next = s.size();
    }
    size_t colon = s.find(':', pos);
    if (colon == std::string::npos || colon >= next) {
      throw std::runtime_error("Invalid memory string: " + s);
    }
    int64_t index = std::stoll(s.substr(pos, colon - pos));
    Number value = Number(s.substr(colon + 1, next - colon - 1));
    set(index, value);
    pos = next + 1;
  }
}

void throwNegativeIndexError(int64_t index) {
  throw std::runtime_error("Memory access with negative index: " +
                           std::to_string(index));
}

Number Memory::get(int64_t index) const {
  if (index >= 0 && index < MEMORY_CACHE_SIZE) {
    return cache[index];
  }
  if (index < 0) {
    throwNegativeIndexError(index);
  }
  auto it = full.find(index);
  if (it != full.end()) {
    return it->second;
  }
  return Number::ZERO;
}

void Memory::set(int64_t index, const Number &value) {
  if (index >= 0 && index < MEMORY_CACHE_SIZE) {
    cache[index] = value;
  } else if (index < 0) {
    throwNegativeIndexError(index);
  } else {
    if (value == Number::ZERO) {
      full.erase(index);
    } else {
      full[index] = value;
    }
  }
}

void Memory::clear() {
  cache.fill(0);
  full.clear();
}

inline std::pair<int64_t, int64_t> getRange(int64_t start, int64_t length) {
  if (length > 0) {
    return {start, start + length};
  } else {
    return {start + length + 1, start + 1};
  }
}

void Memory::clear(int64_t start, int64_t length) {
  auto range = getRange(start, length);
  for (int64_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (i >= range.first && i < range.second) {
      cache[i] = Number::ZERO;
    }
  }
  auto i = full.begin();
  while (i != full.end()) {
    if (i->first >= range.first && i->first < range.second) {
      i = full.erase(i);
    } else {
      i++;
    }
  }
}

void Memory::fill(int64_t start, int64_t length) {
  auto value = get(start);
  auto range = getRange(start, length);
  for (int64_t i = range.first; i < range.second; i++) {
    set(i, value);
  }
}

void Memory::rotateLeft(int64_t start, int64_t length) {
  auto range = getRange(start, length);
  auto leftmost = get(range.first);
  for (int64_t i = range.first; i < range.second - 1; i++) {
    set(i, get(i + 1));
  }
  set(range.second - 1, leftmost);
}

void Memory::rotateRight(int64_t start, int64_t length) {
  auto range = getRange(start, length);
  auto rightmost = get(range.second - 1);
  for (int64_t i = range.second - 1; i > range.first; i--) {
    set(i, get(i - 1));
  }
  set(range.first, rightmost);
}

inline bool collectPositiveAndNegativeValues(int64_t index, const Number &value,
                                             int64_t start, int64_t end,
                                             std::vector<Number> &positive,
                                             std::vector<Number> &negative) {
  if (index >= start && index < end) {
    if (Number::ZERO < value) {
      positive.push_back(value);
    } else if (value < Number::ZERO) {
      negative.push_back(value);
    }
    return true;
  } else {
    return false;
  }
}

Memory Memory::fragment(int64_t start, int64_t length) const {
  Memory frag;
  if (length <= 0) {
    return frag;
  }
  if (length < MEMORY_CACHE_SIZE) {
    for (int64_t i = 0; i < (int64_t)length; i++) {
      frag.set(i, get(start + i));
    }
  } else {
    auto end = start + length;
    for (int64_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
      if (i >= start && i < end) {
        frag.set(i - start, cache[i]);
      }
    }
    auto i = full.begin();
    while (i != full.end()) {
      if (i->first >= start && i->first < end) {
        frag.set(i->first - start, i->second);
      }
      i++;
    }
  }
  return frag;
}

size_t Memory::approximate_size() const {
  return full.size() + MEMORY_CACHE_SIZE;
}

bool Memory::is_less(const Memory &m, int64_t length, bool check_nonn) const {
  if (length <= 0) {
    return false;
  }
  // TODO: this is slow for large lengths
  for (int64_t i = 0; i < (int64_t)length; ++i) {
    auto lhs = get(i);
    if (check_nonn && lhs < 0) {
      return false;
    }
    auto rhs = m.get(i);
    if (lhs < rhs) {
      return true;  // less
    } else if (rhs < lhs) {
      return false;  // greater
    }
  }
  return false;  // equal
}

bool Memory::operator==(const Memory &m) const {
  for (size_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (cache[i] != m.cache[i]) {
      return false;
    }
  }
  for (auto &i : full) {
    if (i.second != 0) {
      auto j = m.full.find(i.first);
      if (j == m.full.end() || i.second != j->second) {
        return false;
      }
    }
  }
  for (auto &i : m.full) {
    if (i.second != 0) {
      auto j = full.find(i.first);
      if (j == full.end() || i.second != j->second) {
        return false;
      }
    }
  }
  return true;  // equal
}

bool Memory::operator!=(const Memory &m) const { return !(*this == m); }

std::ostream &operator<<(std::ostream &out, const Memory &m) {
  std::map<int64_t, Number> sorted;
  for (size_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (m.cache[i] != Number::ZERO) {
      sorted[i] = m.cache[i];
    }
  }
  for (const auto &it : m.full) {
    if (it.second != Number::ZERO) {
      sorted[it.first] = it.second;
    }
  }
  for (const auto &it : sorted) {
    out << it.first << ":" << it.second;
    if (it != *sorted.rbegin()) {
      out << ",";
    }
  }
  return out;
}
