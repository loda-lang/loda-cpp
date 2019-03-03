#pragma once

#include <random>

class Distribution: public std::discrete_distribution<>
{
public:

  Distribution( const std::vector<double>& probabilities );

  static Distribution Uniform( size_t size );

  static Distribution Exponential( size_t size );

  static Distribution Add( const Distribution& d1, const Distribution& d2 );

  static Distribution Mutate( const Distribution& d, std::mt19937& gen );

  void Print() const;

};
