#pragma once

#include <map>
#include <unordered_set>

#include "base/uid.hpp"
#include "math/sequence.hpp"

class ManagedSequence {
 public:
  static const size_t DEFAULT_SEQ_LENGTH;

  static const size_t EXTENDED_SEQ_LENGTH;

  static const size_t FULL_SEQ_LENGTH;

  // required number of terminating terms for programs
  // with exponential runtime complexity
  static const size_t MIN_NUM_EXP_TERMS;

  static bool isTooBig(const Number& n);

  ManagedSequence(UID id = UID('A', 0));

  ManagedSequence(UID id, const std::string& name, const Sequence& full);

  static std::string urlStr(UID id);

  std::string getBFilePath() const;

  Sequence getTerms(int64_t max_num_terms = EXTENDED_SEQ_LENGTH) const;

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

class OeisSeqList : public std::map<char, std::vector<ManagedSequence>> {
 public:
  bool exists(UID id) const;

  const ManagedSequence& get(UID id) const;

  ManagedSequence& get(UID id);

  void add(ManagedSequence&& seq);
};
