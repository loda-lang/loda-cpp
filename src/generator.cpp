#include "generator.hpp"

#include "optimizer.hpp"

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
    p[i] = (p1[i] + p2[i]) / 2;
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

void MutateDist( std::discrete_distribution<>& d, std::mt19937& gen )
{
  std::vector<double> x;
  x.resize( 100, 1 );
  std::discrete_distribution<> v( x.begin(), x.end() );

  auto p1 = d.probabilities();
  std::vector<double> p2( p1.size() );
  for ( size_t i = 0; i < p1.size(); i++ )
  {
    p2[i] = p1[i] + (static_cast<double>( v( gen ) ) / x.size());
//    std::cout << p2[i] << std::endl;
  }
  d = std::discrete_distribution<>( p2.begin(), p2.end() );
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

Generator::Generator( size_t numStates, int64_t seed )
{
  states.resize( numStates );
  for ( Value state = 0; state < numStates; state++ )
  {
    states[state] = State( numStates );
  }
  setSeed( seed );
}

Generator Generator::operator+( const Generator& other )
{
  Generator r( states.size(), gen() );
  for ( Value s = 0; s < states.size(); s++ )
  {
    r.states[s] = states[s] + other.states[s];
  }
  return r;
}

void Generator::mutate( double delta )
{
  for ( auto& s : states )
  {
    MutateDist( s.operationDist, gen );
    MutateDist( s.targetTypeDist, gen );
    MutateDist( s.targetValueDist, gen );
    MutateDist( s.sourceTypeDist, gen );
    MutateDist( s.sourceValueDist, gen );
    MutateDist( s.transitionDist, gen );
    MutateDist( s.positionDist, gen );
  }
}

void Generator::generateOperations( Seed& seed )
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
    if ( targetType == Operand::Type::CONSTANT )
    {
      targetType = Operand::Type::MEM_ACCESS_DIRECT;
    }
    seed.ops.emplace_back( Operation::UPtr( new Mov( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    break;
  case 1:
    if ( targetType == Operand::Type::CONSTANT )
    {
      targetType = Operand::Type::MEM_ACCESS_DIRECT;
    }
    seed.ops.emplace_back( Operation::UPtr( new Add( { targetType, targetValue }, { sourceType, sourceValue } ) ) );
    break;
  case 2:
    if ( targetType == Operand::Type::CONSTANT )
    {
      targetType = Operand::Type::MEM_ACCESS_DIRECT;
    }
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

Program::UPtr Generator::generateProgram( size_t initialState )
{
  Program::UPtr p( new Program() );
  Seed seed;
  seed.state = initialState;
  for ( size_t i = 0; i < 40; i++ )
  {
    generateOperations( seed );
    size_t position = (seed.position * (p->ops.size()));
    for ( size_t j = 0; j < seed.ops.size(); j++ )
    {
      p->ops.emplace( p->ops.begin() + position + j, std::move( seed.ops[j] ) );
    }
  }
  Optimizer o;
  o.Optimize( *p );
  return p;
}

void Generator::setSeed( int64_t seed )
{
  gen.seed( seed );
}
