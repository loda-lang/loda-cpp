#include "lang/memory.hpp"

#include <stdexcept>
#include <string>

#include "lang/number.hpp"

Memory::Memory() { cache.fill(0); }

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
  int64_t end = start + length;
  if (start > end) {
    std::swap(start, end);
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
  // find last non-zero entry
  int64_t max_index = -1;
  for (size_t i = 0; i < MEMORY_CACHE_SIZE; i++) {
    if (m.cache[i] != Number::ZERO) {
      max_index = std::max<int64_t>(max_index, i);
    }
  }
  for (const auto &it : m.full) {
    if (it.second != Number::ZERO) {
      max_index = std::max(max_index, it.first);
    }
  }
  // print all entries from zero to max_index
  out << "[";
  for (int64_t i = 0; i <= max_index; ++i) {
    if (i != 0) out << ",";
    if (i >= 100) {  // limit output
      out << "...";
      break;
    }
    out << m.get(i);
  }
  out << "]";
  return out;
}
