#include "generator_v3.hpp"

#include "stats.hpp"

GeneratorV3::GeneratorV3( const Settings &settings, int64_t seed )
    :
    Generator( settings, seed )
{
  Stats stats;
  stats.load( "stats" );

  size_t i;
  std::vector<double> probs;

  // program length distribution
  probs.resize( stats.num_programs_per_length.size() );
  for ( i = 0; i < stats.num_programs_per_length.size(); i++ )
  {
    probs[i] = stats.num_programs_per_length[i];
  }
  length_dist = std::discrete_distribution<>( probs.begin(), probs.end() );
}

Program GeneratorV3::generateProgram()
{
  Program p;
  // int64_t length = length_dist( gen );

  // TODO

  applyPostprocessing( p );
  return p;
}
