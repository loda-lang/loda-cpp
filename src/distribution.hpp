#pragma once

#include <random>

class Distribution: public std::discrete_distribution<>
{
public:

  Distribution( const std::vector<double>& probabilities );

  static Distribution uniform( size_t size );

  static Distribution exponential( size_t size );

  static Distribution add( const Distribution& d1, const Distribution& d2 );

  static Distribution mutate( const Distribution& d, std::mt19937& gen );

  void print() const;

};
