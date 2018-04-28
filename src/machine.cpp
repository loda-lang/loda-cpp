#include "machine.hpp"

#define VALUE_RANGE 10
#define POSITION_RANGE 100

State::State()
    : State( 0 )
{
}

std::discrete_distribution<> EqualDist( size_t size )
{
  std::vector<double> p;
  p.resize( size, 100.0 );
  return std::discrete_distribution<>( p.begin(), p.end() );
}

std::discrete_distribution<> AddDist( const std::discrete_distribution<>& d1, const std::discrete_distribution<>& d2 )
{
  auto p1 = d1.probabilities();
  auto p2 = d2.probabilities();
  if ( p1.size() != p2.size() )
  {
    throw std::runtime_error( "incompatibe distributions" );
  }
  std::vector<double> p( p1.size() );
  for ( size_t i = 0; i < p.size(); i++ )
  {
    p[i] = p1[i] + p2[i];
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

State::State( size_t numStates )
    : operationDist( EqualDist( 4 ) ),
      targetTypeDist( EqualDist( 3 ) ),
      targetValueDist( EqualDist( VALUE_RANGE ) ),
      sourceTypeDist( EqualDist( 3 ) ),
      sourceValueDist( EqualDist( VALUE_RANGE ) ),
      transitionDist( EqualDist( numStates ) ),
      positionDist( EqualDist( POSITION_RANGE ) )
{
}

State State::operator+( const State& other )
{
  State r;
  r.operationDist = AddDist( operationDist, other.operationDist );
  r.targetTypeDist = AddDist( targetTypeDist, other.targetTypeDist );
  r.sourceTypeDist = AddDist( sourceTypeDist, other.sourceTypeDist );
  r.transitionDist = AddDist( transitionDist, other.transitionDist );
  r.positionDist = AddDist( positionDist, other.positionDist );
  return r;
}

Machine::Machine( size_t numStates, int64_t randomSeed )
    : gen( randomSeed )
{
  states.resize( numStates );
  for ( Value state = 0; state < numStates; state++ )
  {
    states[state] = State( numStates );
  }
}

Machine Machine::operator+( const Machine& other )
{
  Machine r( states.size(), gen() );
  for ( Value s = 0; s < states.size(); s++ )
  {
    r.states[s] = states[s] + other.states[s];
  }
  return r;
}

void Machine::generateOperations( Seed& seed )
{
  auto& s = states.at( seed.state );
  Operand::Type targetType, sourceType;
  switch ( s.targetTypeDist( gen ) )
  {
  case 0:
    targetType = Operand::Type::CONSTANT;
    break;
  case 1:
    targetType = Operand::Type::MEM_ACCESS_DIRECT;
    break;
  case 2:
    targetType = Operand::Type::MEM_ACCESS_INDIRECT;
    break;
  }
  switch ( s.sourceTypeDist( gen ) )
  {
  case 0:
    sourceType = Operand::Type::CONSTANT;
    break;
  case 1:
    sourceType = Operand::Type::MEM_ACCESS_DIRECT;
    break;
  case 2:
    sourceType = Operand::Type::MEM_ACCESS_INDIRECT;
    break;
  }
  Value targetValue = s.targetValueDist( gen );
  Value sourceValue = s.sourceValueDist( gen );

  seed.ops.clear();
  switch ( s.operationDist( gen ) )
  {
  case 0:
    seed.ops.emplace_back( Operation::UPtr( new Mov( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    break;
  case 1:
    seed.ops.emplace_back( Operation::UPtr( new Add( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    break;
  case 2:
    seed.ops.emplace_back( Operation::UPtr( new Sub( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    break;
  case 3:
    seed.ops.emplace_back(
        Operation::UPtr( new LoopBegin( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    seed.ops.emplace_back( Operation::UPtr( new LoopEnd() ) );
    break;
  }
  seed.position = static_cast<double>( s.positionDist( gen ) ) / POSITION_RANGE;
  seed.state = s.transitionDist( gen );
}

Program::UPtr Machine::generateProgram( size_t initialState )
{
  Program::UPtr p( new Program() );
  Seed seed;
  seed.state = initialState;
  for ( size_t i = 0; i < 100; i++ )
  {
    generateOperations( seed );
    size_t position = (seed.position * (p->ops.size()));
    for ( size_t j = 0; j < seed.ops.size(); j++ )
    {
      p->ops.emplace( p->ops.begin() + position + j, std::move( seed.ops[j] ) );
    }
  }
  return p;
}
