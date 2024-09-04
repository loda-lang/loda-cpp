#include "math/sequence.hpp"

#include <sstream>
#include <unordered_set>

Sequence::Sequence(const std::vector<int64_t> &s) {
  const auto t = s.size();
  resize(t);
  for (size_t i = 0; i < t; i++) {
    (*this)[i] = Number(s[i]);
  }
}

Sequence Sequence::subsequence(size_t start, size_t length) const {
  Sequence s;
  if (start < size() && length > 0) {
    const auto new_size = std::min(length, size() - start);
    s.resize(new_size);
    for (size_t i = 0; i < new_size; i++) {
      s[i] = (*this)[start + i];
    }
  }
  return s;
}

bool Sequence::is_linear(size_t start) const {
  if (start + 3 > size()) {
    return false;
  }
  auto d = (*this)[start + 1];
  d -= (*this)[start];
  for (size_t i = start + 2; i < size(); ++i) {
    auto a = (*this)[i - 1];
    a += d;
    if (a != (*this)[i]) {
      return false;
    }
  }
  return true;
}

int64_t Sequence::get_first_delta_lt(const Number &d) const {
  for (size_t i = 1; i < size(); i++) {
    auto delta = (*this)[i];
    delta -= (*this)[i - 1];
    if (delta < d) {
      return i;
    }
  }
  return -1;
}

bool Sequence::align(const Sequence &s, int64_t max_offset) {
  // check if they agree on prefix already
  size_t min_length = std::min(size(), s.size());
  bool agree = true;
  for (size_t i = 0; i < min_length; ++i) {
    if ((*this)[i] != s[i]) {
      agree = false;
      break;
    }
  }
  if (agree) {
    return true;
  }

  // try to align them
  max_offset = std::abs(max_offset);
  for (int64_t offset = 1; offset <= max_offset; ++offset) {
    if (offset >= (int64_t)min_length) {
      break;
    }
    bool agree_pos = true;
    bool agree_neg = true;
    for (size_t i = 0; i < min_length; ++i) {
      if (i + offset < size() && (*this)[i + offset] != s[i]) {
        agree_pos = false;
      }
      if (i + offset < s.size() && (*this)[i] != s[i + offset]) {
        agree_neg = false;
      }
    }
    if (agree_pos) {
      erase(begin(), begin() + offset);
      return true;
    }
    if (agree_neg) {
      insert(begin(), s.begin(), s.begin() + offset);
      return true;
    }
  }
  return false;
}

bool Sequence::operator==(const Sequence &m) const {
  if (size() != m.size()) {
    return false;
  }
  for (size_t i = 0; i < size(); i++) {
    if ((*this)[i] != m[i]) {
      return false;  // not equal
    }
  }
  return true;
}

bool Sequence::operator!=(const Sequence &m) const { return !((*this) == m); }

std::ostream &operator<<(std::ostream &out, const Sequence &seq) {
  for (size_t i = 0; i < seq.size(); i++) {
    if (i > 0) out << ",";
    out << seq[i];
  }
  return out;
}

std::string Sequence::to_string() const {
  std::stringstream ss;
  ss << (*this);
  return ss.str();
}

void Sequence::to_b_file(std::ostream &out, int64_t offset) const {
  for (size_t i = 0; i < size(); i++) {
    out << (offset + i) << " " << (*this)[i] << "\n";
  }
}

std::size_t SequenceHasher::operator()(const Sequence &s) const {
  auto seed = s.size();
  for (auto &n : s) {
    seed ^= n.hash() + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

void SequenceToIdsMap::remove(Sequence seq, size_t id) {
  auto ids = find(seq);
  if (ids != end()) {
    auto it = ids->second.begin();
    while (it != ids->second.end()) {
      if (*it == id) {
        it = ids->second.erase(it);
      } else {
        ++it;
      }
    }
  }
}
