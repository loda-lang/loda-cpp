#include "distribution.hpp"

#include <iostream>
#include <stdexcept>

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
