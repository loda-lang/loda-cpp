#pragma once

#include <map>
#include <unordered_set>

#include "base/uid.hpp"
#include "math/sequence.hpp"
#include "seq/seq_util.hpp"

class ManagedSequence {
 public:
  ManagedSequence(UID id = UID());

  ManagedSequence(UID id, const std::string& name, const Sequence& full);

  std::string getBFilePath() const;

  Sequence getTerms(
      int64_t max_num_terms = SequenceUtil::EXTENDED_SEQ_LENGTH) const;

  size_t numExistingTerms() const { return terms.size(); }

  std::string string() const;

  friend std::ostream& operator<<(std::ostream& out, const ManagedSequence& s);

  UID id;
  std::string name;
  int64_t offset;

 private:
  mutable Sequence terms;
  mutable size_t num_bfile_terms;

  Sequence loadBFile() const;
  void removeInvalidBFile(const std::string& error = "invalid") const;
};
