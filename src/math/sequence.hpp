#pragma once

#include <unordered_map>
#include <vector>

#include "math/number.hpp"

class Sequence : public std::vector<Number> {
 public:
  Sequence() = default;

  Sequence(const Sequence &s) = default;

  Sequence(const std::vector<int64_t> &s);

  Sequence subsequence(size_t start, size_t length) const;

  bool is_linear(size_t start) const;

  int64_t get_first_delta_lt(const Number &d) const;

  bool align(const Sequence &s, int64_t max_offset);

  bool operator==(const Sequence &s) const;

  bool operator!=(const Sequence &s) const;

  friend std::ostream &operator<<(std::ostream &out, const Sequence &s);

  void to_b_file(std::ostream &out, int64_t offset) const;

  std::string to_string() const;
};

struct SequenceHasher {
  std::size_t operator()(const Sequence &s) const;
};

class SequenceToIdsMap
    : public std::unordered_map<Sequence, std::vector<size_t>, SequenceHasher> {
 public:
  void remove(Sequence seq, size_t id);
};
