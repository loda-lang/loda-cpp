#include "base/uid.hpp"

#include <functional>
#include <iomanip>
#include <sstream>
#include <stdexcept>

// Helper: throw an exception for invalid UID construction
inline void invalidate(const std::string& key, const std::string& value) {
  throw std::invalid_argument("Invalid UID " + key + ": " + value);
}

UID::UID(char domain, int64_t number) { set(domain, number); }

UID::UID(const std::string& s) {
  if (s.size() < 2 || s.size() > 7 || s[0] < 'A' || s[0] > 'Z') {
    invalidate("string", "'" + s + "'");
  }
  int64_t number = 0;
  for (size_t i = 1; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') {
      invalidate("string", "'" + s + "'");
    }
    number = number * 10 + (s[i] - '0');
  }
  set(s[0], number);
}

void UID::set(char domain, int64_t number) {
  if (domain < 'A' || domain > 'Z') {
    invalidate("domain", "'" + std::string(1, domain) + "'");
  }
  if (number < 0 || number > 999999) {
    invalidate("number", std::to_string(number));
  }
  value = (static_cast<uint64_t>(domain - 'A') << 48) |
          (number & 0x0000FFFFFFFFFFFF);
}

char UID::domain() const {
  return 'A' + static_cast<char>((value >> 48) & 0xFF);
}

int64_t UID::number() const {
  return static_cast<int64_t>(value & 0x0000FFFFFFFFFFFF);
}

int64_t UID::castToInt() const { return value; }

UID UID::castFromInt(int64_t value) {
  char domain = 'A' + static_cast<char>((value >> 48) & 0xFF);
  int64_t number = static_cast<int64_t>(value & 0x0000FFFFFFFFFFFF);
  return UID(domain, number);
}

UID& UID::operator++(int) {
  set(domain(), number() + 1);
  return *this;
}

std::string UID::string() const {
  std::stringstream s;
  s << std::string(1, domain()) << std::setw(6) << std::setfill('0')
    << number();
  return s.str();
}

bool UIDSet::exists(UID uid) const {
  auto it = data.find(uid.domain());
  if (it == data.end()) {
    return false;
  }
  const auto& flags = it->second;
  if (uid.number() < 0 || uid.number() >= static_cast<int64_t>(flags.size())) {
    return false;
  }
  return flags[uid.number()];
}

void UIDSet::insert(UID uid) {
  auto& flags = data[uid.domain()];
  if (uid.number() >= static_cast<int64_t>(flags.size())) {
    flags.resize(static_cast<int64_t>(1.5 * uid.number()) + 1, false);
  }
  flags[uid.number()] = true;
}
