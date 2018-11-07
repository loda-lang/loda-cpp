#pragma once

#include "number.hpp"

class Test
{
public:

  void fibonacci();

  void exponentiation();

  void num_divisors();

  void ackermann();

  void find();

  void iterate();

  void serialize();

  void oeis();

  void primes();

  void all();

private:

  void testSeq( const std::string& func, const std::string& file, const Sequence& values );

  void testBinary( const std::string& func, const std::string& file,
      const std::vector<std::vector<number_t> >& values );

};
