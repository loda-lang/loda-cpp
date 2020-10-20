#pragma once

#include "interpreter.hpp"
#include "matcher.hpp"
#include "number.hpp"
#include "optimizer.hpp"

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

  void remove( const Sequence &norm_seq, size_t id );

  Matcher::seq_programs_t findSequence( const Program &p, Sequence &norm_seq,
      const std::vector<OeisSequence> &sequences ) const;

  std::vector<std::unique_ptr<Matcher>>& getMatchers()
  {
    return matchers;
  }

  std::vector<MatcherStats>& getMatcherStats()
  {
    return matcher_stats;
  }

  void publishMetrics();

private:

  void findAll( const Program &p, const Sequence &norm_seq, const std::vector<OeisSequence> &sequences,
      Matcher::seq_programs_t &result ) const;

  const Settings &settings;
  Interpreter interpreter;
  Optimizer optimizer;
  std::vector<std::unique_ptr<Matcher>> matchers;
  mutable std::vector<MatcherStats> matcher_stats;
  mutable size_t num_find_attempts;

};