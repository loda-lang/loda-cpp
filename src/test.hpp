#pragma once

#include "number.hpp"

class Test
{
public:

  void fibonacci();

  void exponentiation();

  void ackermann();

  void find();

  void iterate();

  void serialize();

  void oeis();

  void all();

private:

  void testBinary( const std::string& func, const std::string& file, const std::vector<std::vector<number_t> >& values );

};
