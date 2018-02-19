#include "generator.hpp"

#include <random>

Program::UPtr Generator::Generate( Machine& m )
{
  Program::UPtr p( new Program() );

  std::default_random_engine rand_engine;
  std::uniform_int_distribution<int> distribution(1,6);
  int dice_roll = distribution( rand_engine );

  while ( p->ops.size() < m.maxOperations )
  {


  }

  return p;
}
