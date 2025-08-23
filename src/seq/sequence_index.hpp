#pragma once

#include <map>
#include <vector>

#include "seq/managed_sequence.hpp"

class SequenceIndex : public std::map<char, std::vector<ManagedSequence>> {
 public:
  bool exists(UID id) const;

  const ManagedSequence& get(UID id) const;

  ManagedSequence& get(UID id);

  void add(ManagedSequence&& seq);
};
