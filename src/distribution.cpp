#include "distribution.hpp"

#include <iostream>
#include <stdexcept>

Distribution::Distribution( const std::vector<double>& probabilities )
    : std::discrete_distribution<>( probabilities.begin(), probabilities.end() )
{
}

Distribution Distribution::Uniform( size_t size )
{
  return Distribution( std::vector<double>( size, 100.0 ) );
}

Distribution Distribution::Exponential( size_t size )
{
  std::vector<double> probabilities( size );
  double v = 1.0;
  for ( int i = size - 1; i >= 0; --i )
  {
    probabilities[i] = v;
    v *= 2.0;
  }
  return Distribution( probabilities );
}

Distribution Distribution::Add( const Distribution& d1, const Distribution& d2 )
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
  return Distribution( p );
}

Distribution Distribution::Mutate( const Distribution& d, std::mt19937& gen )
{
  std::vector<double> x;
  x.resize( 100, 1 );
  std::discrete_distribution<> v( x.begin(), x.end() );
  auto p1 = d.probabilities();
  std::vector<double> p2( p1.size() );
  for ( size_t i = 0; i < p1.size(); i++ )
  {
    p2[i] = p1[i] + ((static_cast<double>( v( gen ) ) / x.size()) / 50);
  }
  return Distribution( p2 );
}

void Distribution::Print() const
{
  auto probs = probabilities();
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
