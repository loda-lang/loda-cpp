#pragma once

#include <memory>

#include "eval/evaluator.hpp"
#include "eval/minimizer.hpp"
#include "mine/matcher.hpp"

class OeisSequence;

struct check_result_t {
  std::string status;
  Program program;
};

class Finder {
 public:
  Finder(const Settings &settings, Evaluator &evaluator);

  virtual ~Finder() {}

  void insert(const Sequence &norm_seq, size_t id);

  void remove(const Sequence &norm_seq, size_t id);

  Matcher::seq_programs_t findSequence(
      const Program &p, Sequence &norm_seq,
      const std::vector<OeisSequence> &sequences);

  check_result_t checkProgramExtended(Program program, Program existing,
                                      bool is_new, const OeisSequence &seq,
                                      bool full_check, size_t num_usages);

  check_result_t checkProgramBasic(const Program &program,
                                   const Program &existing, bool is_new,
                                   const OeisSequence &seq,
                                   const std::string &change_type,
                                   size_t previous_hash, bool full_check,
                                   size_t num_usages);

  std::vector<std::unique_ptr<Matcher>> &getMatchers() { return matchers; }

  void logSummary(size_t loaded_count);

  std::string isOptimizedBetter(Program existing, Program optimized,
                                const OeisSequence &seq, bool full_check,
                                size_t num_usages);

  std::string compare(Program p1, Program p2, const std::string &name1,
                      const std::string &name2, const OeisSequence &seq,
                      size_t num_terms, size_t num_usages);

 private:
  static constexpr double THRESHOLD_BETTER = 1.05;
  static constexpr double THRESHOLD_FASTER = 1.1;

  void findAll(const Program &p, const Sequence &norm_seq,
               const std::vector<OeisSequence> &sequences,
               Matcher::seq_programs_t &result);

  void notifyInvalidMatch(size_t id);

  void notifyUnfoldOrMinimizeProblem(const Program &p, const std::string &id);

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
