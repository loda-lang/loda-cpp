#pragma once

#include <string>
#include <vector>

#include "value.hpp"

class Test
{
public:

  void All();

  void Fibonacci();

  void Exponentiation();

  void Ackermann();

private:

  void TestBinary( const std::string& func, const std::string& file, const std::vector<std::vector<Value> >& values );

};
