#pragma once

#include "number.hpp"

class Test
{
public:

  void fibonacci();

  void exponentiation();

  void num_divisors();

  void ackermann();

  void iterate();

  void serialize();

  void oeis();

  void primes();

  void primes2();

  void all();

private:

  void testSeq( const std::string& func, const std::string& file, const Sequence& values );

  void testBinary( const std::string& func, const std::string& file,
      const std::vector<std::vector<number_t> >& values );

};
