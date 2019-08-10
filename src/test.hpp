#pragma once

#include "number.hpp"

class Test
{
public:

  void exponentiation();

  void ackermann();

  void iterate();

  void oeis();

  void primes();

  void primes2();

  void optimize();

  void matcher();

  void synthesizer();

  void all();

private:

  void testSeq( const std::string& func, const std::string& file, const Sequence& values );

  void testBinary( const std::string& func, const std::string& file,
      const std::vector<std::vector<number_t> >& values );

};
