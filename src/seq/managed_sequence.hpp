#pragma once

#include <map>
#include <unordered_set>

#include "base/uid.hpp"
#include "math/sequence.hpp"
#include "seq/sequence_util.hpp"

class ManagedSequence {
 public:
  ManagedSequence(UID id = UID('A', 0));

  ManagedSequence(UID id, const std::string& name, const Sequence& full);

  std::string getBFilePath() const;

  Sequence getTerms(
      int64_t max_num_terms = SequenceUtil::EXTENDED_SEQ_LENGTH) const;

  size_t existingNumTerms() const { return terms.size(); }

  UID id;
  std::string name;
  int64_t offset;

  friend std::ostream& operator<<(std::ostream& out, const ManagedSequence& s);

  std::string to_string() const;

 private:
  mutable Sequence terms;
  mutable size_t num_bfile_terms;
};
