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

  void exponentiation();

  void ackermann();

  void iterate();

  void optimize();

  void matcher();

  void synthesizer( size_t degree );

  void all();

private:

  volatile sig_atomic_t &exit_flag_;

  void testSeq( const std::string &func, const std::string &file, const Sequence &values );

  void testBinary( const std::string &func, const std::string &file,
      const std::vector<std::vector<number_t> > &values );

};
