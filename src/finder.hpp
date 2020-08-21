#pragma once

#include "interpreter.hpp"
#include "matcher.hpp"

class OeisSequence;

struct MatcherStats
{
  size_t candidates;
  size_t false_positives;
  size_t errors;
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
  std::vector<std::unique_ptr<Matcher>> matchers;
  mutable std::vector<MatcherStats> matcher_stats;
  mutable size_t num_find_attempts;

};
