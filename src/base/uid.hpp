#pragma once

#include <cstdint>
#include <string>

// UID represents a unique identifier for sequences/programs, e.g., "A123456"
class UID {
 public:
  // Default constructor initializes to "A000000"
  UID() : UID('A', 0) {}

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

  // Return the internal integer representation of the UID
  int64_t castToInt() const;

  // Create a UID from its internal integer representation
  static UID castFromInt(int64_t value);

  // Return the UID as a string, e.g., "A123456" (always 7 chars)
  std::string string() const;

  bool operator==(const UID& other) const { return value == other.value; }
  bool operator!=(const UID& other) const { return value != other.value; }
  bool operator<(const UID& other) const { return value < other.value; }
  bool operator>(const UID& other) const { return value > other.value; }
  bool operator<=(const UID& other) const { return value <= other.value; }
  bool operator>=(const UID& other) const { return value >= other.value; }
  UID& operator++(int);

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
