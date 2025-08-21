#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

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

class UIDSet {
 public:
  class const_iterator {
   public:
    using map_iter_t = std::map<char, std::vector<bool>>::const_iterator;
    using value_type = UID;
    using reference = UID;
    using pointer = void;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    const_iterator();
    const_iterator(map_iter_t map_it, map_iter_t map_end);
    reference operator*() const;
    const_iterator& operator++();
    bool operator==(const const_iterator& other) const;
    bool operator!=(const const_iterator& other) const;

   private:
    map_iter_t m_map_it{};
    map_iter_t m_map_end{};
    size_t m_vec_idx = 0;
    void advance_to_next_valid();
  };

  const_iterator begin() const;
  const_iterator end() const;
  const_iterator cbegin() const;
  const_iterator cend() const;

  bool empty() const;
  bool exists(UID uid) const;
  void insert(UID uid);
  void erase(UID uid);
  void clear();

  bool operator==(const UIDSet& other) const { return data == other.data; }
  bool operator!=(const UIDSet& other) const { return data != other.data; }

 private:
  std::map<char, std::vector<bool>> data;
};
