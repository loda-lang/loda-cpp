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

  void iterator();

  void optimizer( size_t tests );

  void polynomial_matcher( size_t tests, size_t degree );

  void polynomial_synthesizer( size_t tests, size_t degree );

private:

  volatile sig_atomic_t &exit_flag_;

  void testSeq( const std::string &func, const std::string &file, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file,
      const std::vector<std::vector<number_t> > &values );

};
