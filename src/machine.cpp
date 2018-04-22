#include "machine.hpp"

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
      targetDist( EqualDist( 3 ) ),
      sourceDist( EqualDist( 3 ) ),
      transitionDist( EqualDist( numStates ) ),
      positionDist( EqualDist( 100 ) )
{
}

State State::operator+( const State& other )
{
  State r;
  r.operationDist = AddDist( operationDist, other.operationDist );
  r.targetDist = AddDist( targetDist, other.targetDist );
  r.sourceDist = AddDist( sourceDist, other.sourceDist );
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

std::pair<Operation::UPtr, size_t> Machine::generateOperation( size_t state )
{
  auto& s = states.at( state );
  Operation::UPtr op;
  Operand::Type targetType, sourceType;
  switch ( s.targetDist( gen ) )
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
  switch ( s.sourceDist( gen ) )
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
  Value sourceValue = 1;
  Value targetValue = 1;

  switch ( s.operationDist( gen ) )
  {
  case 0:
    op.reset( new Mov( { targetType, targetValue }, { sourceType, sourceValue } ) );
    break;
  case 1:
    op.reset( new Add( { targetType, targetValue }, { sourceType, sourceValue } ) );
    break;
  case 2:
    op.reset( new Sub( { targetType, targetValue }, { sourceType, sourceValue } ) );
    break;
  }

  size_t nextState = s.transitionDist( gen );

  return std::make_pair<Operation::UPtr, size_t>( std::move( op ), std::move( nextState ) );
}

Program::UPtr Machine::generateProgram( size_t initialState )
{
  Program::UPtr p( new Program() );
  std::pair<Operation::UPtr, size_t> next;
  next.second = initialState;
  for ( size_t i = 0; i < 10; i++ )
  {
    next = generateOperation( next.second );
    p->ops.emplace_back( std::move( next.first ) );
  }
  return p;
}
