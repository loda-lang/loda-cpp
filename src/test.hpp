#pragma once

#include "common.hpp"

class Test
{
public:

  void Fibonacci();

  void Exponentiation();

  void Ackermann();

  void Find();

  void Iterate();

  void Serialize();

  void All();

private:

  void TestBinary( const std::string& func, const std::string& file, const std::vector<std::vector<number_t> >& values );

};
