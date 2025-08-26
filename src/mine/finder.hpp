#pragma once

#include <memory>

#include "base/uid.hpp"
#include "eval/evaluator.hpp"
#include "eval/minimizer.hpp"
#include "mine/checker.hpp"
#include "mine/matcher.hpp"
#include "oeis/invalid_matches.hpp"
#include "seq/seq_index.hpp"

class ManagedSequence;

class Finder {
 public:
  Finder(const Settings &settings, Evaluator &evaluator);

  virtual ~Finder() {}

  void insert(const Sequence &norm_seq, UID id);

  void remove(const Sequence &norm_seq, UID id);

  Matcher::seq_programs_t findSequence(const Program &p, Sequence &norm_seq,
                                       const SequenceIndex &sequences);

  std::vector<std::unique_ptr<Matcher>> &getMatchers() { return matchers; }

  Checker &getChecker() { return checker; }

  void logSummary(size_t loaded_count);

 private:
  void findAll(const Program &p, const Sequence &norm_seq,
               const SequenceIndex &sequences, Matcher::seq_programs_t &result);

  void notifyUnfoldOrMinimizeProblem(const Program &p, const std::string &id);

  const Settings &settings;
  Evaluator &evaluator;  // shared instance to save memory
  Optimizer optimizer;
  Minimizer minimizer;
  std::vector<std::unique_ptr<Matcher>> matchers;
  mutable size_t num_find_attempts;
  InvalidMatches invalid_matches;
  Checker checker;

  // temporary containers (cached as members)
  mutable std::unordered_set<int64_t> tmp_used_cells;
  mutable std::vector<Sequence> tmp_seqs;
  mutable Matcher::seq_programs_t tmp_result;
  mutable std::map<std::string, std::string> tmp_matcher_labels;
};
