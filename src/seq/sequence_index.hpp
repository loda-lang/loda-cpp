#pragma once

#include <map>
#include <vector>

#include "seq/managed_sequence.hpp"

class SequenceIndex {
 public:
  bool exists(UID id) const;

  const ManagedSequence& get(UID id) const;

  ManagedSequence& get(UID id);

  void add(ManagedSequence&& seq);

  class const_iterator;
  const_iterator begin() const;
  const_iterator end() const;

 private:
  std::map<char, std::vector<ManagedSequence>> data;
};

class SequenceIndex::const_iterator {
 public:
  using outer_iter_t =
      std::map<char, std::vector<ManagedSequence>>::const_iterator;
  using inner_iter_t = std::vector<ManagedSequence>::const_iterator;

  const_iterator() = default;
  const_iterator(outer_iter_t outer, outer_iter_t outer_end);

  const ManagedSequence& operator*() const;
  const ManagedSequence* operator->() const;
  const_iterator& operator++();
  bool operator==(const const_iterator& other) const;
  bool operator!=(const const_iterator& other) const;

 private:
  void advance_to_valid();
  outer_iter_t outer_it;
  outer_iter_t outer_end;
  inner_iter_t inner_it;
  inner_iter_t inner_end;
};
