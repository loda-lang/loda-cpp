#pragma once

#include <unordered_set>

#include "lang/sequence.hpp"

class OeisSequence {
 public:
  static const size_t DEFAULT_SEQ_LENGTH;

  static const size_t EXTENDED_SEQ_LENGTH;

  static const size_t FULL_SEQ_LENGTH;

  static bool isTooBig(const Number& n);

  OeisSequence(size_t id = 0);

  OeisSequence(std::string id_str);

  OeisSequence(size_t id, const std::string& name, const Sequence& full);

  std::string id_str(const std::string& prefix = "A") const;

  std::string dir_str() const;

  std::string url_str() const;

  std::string getProgramPath(bool local = false) const;

  std::string getBFilePath() const;

  Sequence getTerms(int64_t max_num_terms = EXTENDED_SEQ_LENGTH) const;

  size_t existingNumTerms() const { return terms.size(); }

  size_t id;
  std::string name;

  friend std::ostream& operator<<(std::ostream& out, const OeisSequence& s);

  std::string to_string() const;

 private:
  mutable Sequence terms;
  mutable size_t num_bfile_terms;
};
