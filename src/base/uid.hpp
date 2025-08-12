#pragma once

#include <cstdint>
#include <string>

// UID represents a unique identifier for sequences/programs, e.g., "A123456"
class UID {
 public:
  // Construct a UID from a domain character (A-Z) and a 6-digit number
  // (0-999999)
  explicit UID(char domain, int64_t number);

  // Construct a UID from a string, e.g., "A123456" (domain A-Z, 6 digits)
  // Throws std::invalid_argument if the string is not valid.
  explicit UID(const std::string& s);

  // Set the UID value from domain and number, with validation
  void set(char domain, int64_t number);

  // Get the domain character (A-Z) of the UID
  char domain() const;

  // Get the 6-digit number part of the UID
  int64_t number() const;

  // Return the UID as a string, e.g., "A123456" (always 7 chars)
  std::string string() const;

  inline bool operator==(const UID& other) const {
    return value == other.value;
  }

  inline bool operator!=(const UID& other) const {
    return value != other.value;
  }

  inline bool operator<(const UID& other) const { return value < other.value; }

  inline bool operator>(const UID& other) const { return value > other.value; }

  inline bool operator<=(const UID& other) const {
    return value <= other.value;
  }

  inline bool operator>=(const UID& other) const {
    return value >= other.value;
  }

  friend struct std::hash<UID>;

 private:
  uint64_t value;
};

namespace std {
template <>
struct hash<UID> {
  std::size_t operator()(const UID& uid) const noexcept {
    return std::hash<uint64_t>{}(uid.value);
  }
};
}  // namespace std
