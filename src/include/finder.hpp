#pragma once

#include <memory>

#include "evaluator.hpp"
#include "matcher.hpp"
#include "metrics.hpp"
#include "minimizer.hpp"
#include "number.hpp"

class OeisSequence;

struct MatcherStats {
  int64_t candidates;
  int64_t successes;
  int64_t false_positives;
  int64_t errors;
};

class Finder {
 public:
  Finder(const Settings &settings, Evaluator &evaluator);

  virtual ~Finder() {}

  void insert(const Sequence &norm_seq, size_t id);

  Matcher::seq_programs_t findSequence(
      const Program &p, Sequence &norm_seq,
      const std::vector<OeisSequence> &sequences);

  std::pair<std::string, Program> checkProgram(Program program,
                                               Program existing, bool is_new,
                                               const OeisSequence &seq);

  std::vector<std::unique_ptr<Matcher>> &getMatchers() { return matchers; }

  void logSummary(size_t loaded_count);

 private:
  void findAll(const Program &p, const Sequence &norm_seq,
               const std::vector<OeisSequence> &sequences,
               Matcher::seq_programs_t &result);

  void notifyInvalidMatch(size_t id);

  std::pair<bool, Program> checkAndMinimize(Program p, const OeisSequence &seq,
                                            bool compare);

  std::string isOptimizedBetter(Program existing, Program optimized,
                                const OeisSequence &seq);

  const Settings &settings;
  Evaluator &evaluator;  // shared instance to save memory
  Optimizer optimizer;
  Minimizer minimizer;
  std::vector<std::unique_ptr<Matcher>> matchers;
  mutable size_t num_find_attempts;

  std::map<size_t, int64_t> invalid_matches;
  AdaptiveScheduler scheduler;

  // temporary containers (cached as members)
  mutable std::unordered_set<int64_t> tmp_used_cells;
  mutable std::vector<Sequence> tmp_seqs;
  mutable Matcher::seq_programs_t tmp_result;
  mutable std::map<std::string, std::string> tmp_matcher_labels;
};
