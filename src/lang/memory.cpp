#include "lang/memory.hpp"

#include <map>
#include <stdexcept>
#include <string>

#include "lang/number.hpp"

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

void Memory::clear(int64_t start, int64_t length) {
  int64_t end = start + length;  // exclusive
  if (start > end) {
    std::swap(start, end);
    start++;
    end++;
  }
  for (int64_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (i >= start && i < end) {
      cache[i] = Number::ZERO;
    }
  }
  auto i = full.begin();
  while (i != full.end()) {
    if (i->first >= start && i->first < end) {
      i = full.erase(i);
    } else {
      i++;
    }
  }
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

void Memory::sort(int64_t start, int64_t length) {
  // check for negative range
  int64_t end = start + length;  // exclusive
  bool reverse = false;
  if (start > end) {
    std::swap(start, end);
    start++;
    end++;
    reverse = true;
  }
  // collect positive and negative values
  std::vector<Number> positive, negative;
  for (int64_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (collectPositiveAndNegativeValues(i, cache[i], start, end, positive,
                                         negative)) {
      cache[i] = Number::ZERO;
    }
  }
  auto it = full.begin();
  while (it != full.end()) {
    if (collectPositiveAndNegativeValues(it->first, it->second, start, end,
                                         positive, negative)) {
      it = full.erase(it);
    } else {
      it++;
    }
  }
  // sort positive and negative values
  std::sort(positive.begin(), positive.end());
  std::sort(negative.begin(), negative.end());
  // write sorted values back
  size_t i;
  if (reverse) {
    for (i = 0; i < positive.size(); i++) {
      set(start + i, positive[i]);
    }
    for (i = 0; i < negative.size(); i++) {
      set(end - negative.size() + i, negative[i]);
    }
  } else {
    for (i = 0; i < positive.size(); i++) {
      set(end - positive.size() + i, positive[i]);
    }
    for (i = 0; i < negative.size(); i++) {
      set(start + i, negative[i]);
    }
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
