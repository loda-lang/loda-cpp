#pragma once

#include "interpreter.hpp"
#include "matcher.hpp"
#include "number.hpp"

#include <memory>

class OeisSequence;

struct MatcherStats
{
  int64_t candidates;
  int64_t successes;
  int64_t false_positives;
  int64_t errors;
};

class Finder
{
public:

  Finder( const Settings &settings );

  virtual ~Finder()
  {
  }

  void insert( const Sequence &norm_seq, size_t id );

  Matcher::seq_programs_t findSequence( const Program &p, Sequence &norm_seq,
      const std::vector<OeisSequence> &sequences );

  std::vector<std::unique_ptr<Matcher>>& getMatchers()
  {
    return matchers;
  }

  std::vector<MatcherStats>& getMatcherStats()
  {
    return matcher_stats;
  }

  void logSummary( size_t loaded_count );

  void publishMetrics( std::vector<Metrics::Entry>& entries );

private:

  void findAll( const Program &p, const Sequence &norm_seq, const std::vector<OeisSequence> &sequences,
      Matcher::seq_programs_t &result );

  const Settings &settings;
  Interpreter interpreter;
  std::vector<std::unique_ptr<Matcher>> matchers;
  mutable std::vector<MatcherStats> matcher_stats;
  mutable size_t num_find_attempts;

  // temporary containers (cached as members)
  mutable std::unordered_set<int64_t> tmp_used_cells;
  mutable Sequence tmp_full_seq;
  mutable std::vector<Sequence> tmp_seqs;
  mutable Matcher::seq_programs_t tmp_result;
  mutable std::map<std::string, std::string> tmp_matcher_labels;

};
