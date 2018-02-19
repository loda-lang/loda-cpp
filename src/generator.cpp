#include "generator.hpp"

#include <random>

Program::UPtr Generator::Generate( Machine& m )
{
  Program::UPtr p( new Program() );

  std::default_random_engine engine;

  OperandDistribution d;

  while ( p->ops.size() < m.maxOperations )
  {
	  Operation::UPtr op( new Mov(d(engine), d(engine)) );
	  p->ops.emplace_back( std::move( op ) );
  }

  return p;
}
