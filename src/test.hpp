#pragma once

#include "matcher.hpp"
#include "number.hpp"
#include "util.hpp"

class Test
{
public:

  Test( volatile sig_atomic_t &exit_flag )
      :
      exit_flag_( exit_flag )
  {
  }

  void all();

  void fibonacci();

  void ackermann();

  void collatz();

  void optimizer( size_t tests );

  void deltaMatcher();

  void polynomialMatcher( size_t tests, size_t degree );

  void polynomialSynthesizer( size_t tests, size_t degree );

private:

  volatile sig_atomic_t &exit_flag_;

  void testSeq( const std::string &func, const std::string &file, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file,
      const std::vector<std::vector<number_t> > &values );

  void testMatcherSet( Matcher &matcher, const std::vector<number_t> &ids );

  void testMatcherPair( Matcher &matcher, number_t id1, number_t id2 );

};
