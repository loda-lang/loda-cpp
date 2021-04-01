#pragma once

#include "matcher.hpp"
#include "number.hpp"
#include "util.hpp"

class Test
{
public:

  Test()
  {
  }

  void all();

  void semantics();

  void memory();

  void knownPrograms();

  void oeisSeq();

  void steps();

  void ackermann();

  void collatz();

  void optimizer();

  void minimizer( size_t tests );

  void linearMatcher();

  void deltaMatcher();

  void polynomialMatcher( size_t tests, size_t degree );

  void stats();

  void config();

private:

  void testSeq( size_t id, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file,
      const std::vector<std::vector<number_t> > &values );

  void testMatcherSet( Matcher &matcher, const std::vector<size_t> &ids );

  void testMatcherPair( Matcher &matcher, size_t id1, size_t id2 );

};
