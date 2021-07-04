#pragma once

#include "matcher.hpp"
#include "number.hpp"
#include "oeis_manager.hpp"
#include "util.hpp"

#include <random>

class Test
{
public:

  Test( int64_t seed );

  void all();

  void number();

  void randomNumber( size_t tests );

  void semantics();

  void sequence();

  void memory();

  void iterator( size_t tests );

  void knownPrograms();

  void oeisSeq();

  void steps();

  void blocks();

  void ackermann();

  void collatz();

  void optimizer();

  void minimizer( size_t tests );

  void linearMatcher();

  void deltaMatcher();

  void digitMatcher();

  void stats();

  void config();

private:

  std::vector<std::pair<Program, Program>> loadInOutTests( const std::string& prefix );

  void testSeq( size_t id, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file, const std::vector<std::vector<int64_t> > &values );

  void testMatcherSet( Matcher &matcher, const std::vector<size_t> &ids );

  void testMatcherPair( Matcher &matcher, size_t id1, size_t id2 );

  Settings settings;
  OeisManager manager;
  std::mt19937 gen;

};
