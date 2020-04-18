#pragma once

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

  void delta_matcher();

  void delta_matcher_pair( number_t id1, number_t id2 );

  void polynomial_matcher( size_t tests, size_t degree );

  void polynomial_synthesizer( size_t tests, size_t degree );

private:

  volatile sig_atomic_t &exit_flag_;

  void testSeq( const std::string &func, const std::string &file, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file,
      const std::vector<std::vector<number_t> > &values );

};
