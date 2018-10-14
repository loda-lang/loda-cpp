#include "generator.hpp"

#include "number.hpp"
#include "interpreter.hpp"
#include "optimizer.hpp"
#include "printer.hpp"

#include <algorithm>

#define VALUE_RANGE 8
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

std::discrete_distribution<> ExpDist( size_t size )
{
  std::vector<double> p( size );
  double v = 1.0;
  for ( int i = size - 1; i >= 0; --i )
  {
    p[i] = v;
    v *= 2.0;
  }
  return std::discrete_distribution<>( p.begin(), p.end() );
}

void printDist( const std::discrete_distribution<>& d )
{
  auto probs = d.probabilities();
  std::cout << "[";
  for ( size_t i = 0; i < probs.size(); i++ )
  {
    if ( i > 0 )
    {
      std::cout << ",";
    }
    std::cout << probs[i];
  }
  std::cout << "]";
}

void State::print()
{
  std::cout << "[";
  printDist( operationDist );
  std::cout << ",";
  printDist( targetTypeDist );
  std::cout << ",";
  printDist( targetValueDist );
  std::cout << ",";
  printDist( sourceTypeDist );
  std::cout << ",";
  printDist( sourceValueDist );
  std::cout << ",";
  printDist( transitionDist );
  std::cout << ",";
  printDist( positionDist );
  std::cout << "]";
}

void Generator::print()
{
  std::cout << "[" << score << ",";
  for ( size_t i = 0; i < states.size(); i++ )
  {
    if ( i > 0 )
    {
      std::cout << ",";
    }
    states[i].print();
  }
  std::cout << "]";
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
    p2[i] = p1[i] + ((static_cast<double>( v( gen ) ) / x.size()) / 50);
//    std::cout << p2[i] << std::endl;
  }
  d = std::discrete_distribution<>( p2.begin(), p2.end() );
}

State::State( size_t numStates )
    : operationDist( EqualDist( 4 ) ),
      targetTypeDist( EqualDist( 2 ) ),
      targetValueDist( ExpDist( VALUE_RANGE ) ),
      sourceTypeDist( EqualDist( 3 ) ),
      sourceValueDist( ExpDist( VALUE_RANGE ) ),
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

Generator::Generator( const Settings& settings, size_t numStates, int64_t seed )
    : settings( settings )
{
  states.resize( numStates );
  for ( number_t state = 0; state < numStates; state++ )
  {
    states[state] = State( numStates );
  }
  setSeed( seed );
}

Generator Generator::operator+( const Generator& other )
{
  Generator r( settings, states.size(), gen() );
  for ( number_t s = 0; s < states.size(); s++ )
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
    targetType = Operand::Type::MEM_ACCESS_DIRECT;
    break;
  case 1:
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
  number_t targetValue = s.targetValueDist( gen );
  number_t sourceValue = s.sourceValueDist( gen );
  Operand to( targetType, targetValue );
  Operand so( sourceType, sourceValue );

  seed.ops.clear();
  switch ( s.operationDist( gen ) )
  {
  case 0:
    seed.ops.push_back( Operation( Operation::Type::MOV, to, so ) );
    break;
  case 1:
    seed.ops.push_back( Operation( Operation::Type::ADD, to, so ) );
    break;
  case 2:
    seed.ops.push_back( Operation( Operation::Type::SUB, to, so ) );
    break;
  case 3:
    seed.ops.push_back( Operation( Operation::Type::LPB, to, so ) );
    seed.ops.push_back( Operation( Operation::Type::LPE ) );
    break;
  }
  seed.position = static_cast<double>( s.positionDist( gen ) ) / POSITION_RANGE;
  seed.state = s.transitionDist( gen );
}

Program Generator::generateProgram( size_t initialState )
{
  Program p;
  Seed seed;
  seed.state = initialState;
  for ( size_t i = 0; i < settings.num_operations; i++ )
  {
    generateOperations( seed );
    size_t position = (seed.position * (p.ops.size()));
    for ( size_t j = 0; j < seed.ops.size(); j++ )
    {
      p.ops.emplace( p.ops.begin() + position + j, std::move( seed.ops[j] ) );
    }
  }
  return p;
}

void Generator::setSeed( int64_t seed )
{
  gen.seed( seed );
}

Finder::Finder( const Settings& settings )
    : settings( settings )
{
}

Program Finder::find( Scorer& scorer, size_t seed, size_t max_iterations )
{
  Program p;

  size_t states = 10;
  size_t gen_count = 100;
  size_t tries = 100;

//  number_t max_value = 0;
//  for ( auto& v : target )
//  {
//    if ( v > max_value ) max_value = v;
//  }

  std::vector<Generator::UPtr> generators;
  for ( size_t i = 0; i < gen_count; i++ )
  {
    generators.emplace_back( Generator::UPtr( new Generator( settings, states, seed + i ) ) );
  }

  Interpreter interpreter( settings );
//  FixedSequenceScorer scorer( target );
//  Printer printer;

  Sequence s;
  for ( size_t iteration = 0; iteration < max_iterations; iteration++ )
  {
    for ( size_t i = 0; i < gen_count; i++ )
    {
      generators[i]->setSeed( seed + i );
    }

    // evaluate generator and score them
    double avg_score = 0.0;
    for ( auto& gen : generators )
    {
      gen->score = 0;
      for ( size_t i = 0; i < tries; i++ )
      {
        p = gen->generateProgram( 0 );
        try
        {
          s = interpreter.eval( p );
        }
        catch ( const std::exception& e )
        {
          std::cerr << std::string( e.what() ) << std::endl;
//          gen->score += 2 * max_value;
          continue;
        }
        auto score = scorer.score( s );
        if ( score == 0 )
        {
//          std::cout << "Found!" << std::endl;
//          printer.print( p, std::cout );
          return p;
        }
        if ( gen->score == 0 || score << gen->score )
        {
          gen->score = score;
        }
      }
//      std::cout << "Score: " << gen->score << std::endl;
      avg_score += gen->score;
    }
    avg_score = (avg_score / generators.size()) / tries;
//    std::cout << "Average: " << avg_score << "\n" << std::endl;

// sort generators by their scores
    std::stable_sort( generators.begin(), generators.end(), less_than_score );

    // print top ten
    /*   for ( size_t i = 0; i < (gen_count / 10); i++ )
     {
     generators[i]->print();
     std::cout << std::endl;
     }
     std::cout << std::endl;
     */
    // create new generators
    std::vector<Generator::UPtr> new_generators;
    for ( size_t i = 0; i < (gen_count / 10); i++ )
    {
      for ( size_t j = 0; j < (gen_count / 10); j++ )
      {
        Generator::UPtr g( new Generator( *generators[i] + *generators[j] ) );
        g->mutate( 0.5 );
        new_generators.emplace_back( std::move( g ) );
      }
    }
    generators = std::move( new_generators );
  }

}

Scorer::~Scorer()
{
}

FixedSequenceScorer::FixedSequenceScorer( const Sequence& target )
    : target_( target )
{
}

FixedSequenceScorer::~FixedSequenceScorer()
{
}

number_t FixedSequenceScorer::score( const Sequence& s )
{
  number_t score = 0;
  auto size = target_.size();
  for ( number_t i = 0; i < size; i++ )
  {
    auto v1 = s.at( i );
    auto v2 = target_.at( i );
    score += (v1 > v2) ? (v1 - v2) : (v2 - v1);
  }
  return score;
}
