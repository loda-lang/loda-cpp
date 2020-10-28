#include "generator_v3.hpp"

#include "stats.hpp"

inline size_t getIndex( size_t pos, size_t len )
{
  if ( pos >= len )
  {
  }
  return (((len - 1) * len) / 2) + pos;
}

GeneratorV3::GeneratorV3( const Settings &settings, int64_t seed )
    :
    Generator( settings, seed )
{
  Stats stats;
  stats.load( "stats" );

  size_t i;
  std::vector<double> probs;

  // resize operation distribution vector
  size_t max_len = 0;
  for ( size_t len = 0; len < stats.num_programs_per_length.size(); len++ )
  {
    if ( stats.num_programs_per_length[len] > 0 )
    {
      max_len = len;
    }
  }
  operation_dists.resize( getIndex( max_len - 1, max_len ) + 1 );

  // initialize operation distributions
  OpProb p;
  for ( auto &it : stats.num_operation_positions )
  {
    i = getIndex( it.first.pos, it.first.len );
    auto &op_dist = operation_dists.at( i );
    p.operation = it.first.op;
    p.partial_sum = it.second;
    if ( !op_dist.empty() )
    {
      p.partial_sum += op_dist.back().partial_sum;
    }
    op_dist.push_back( p );
  }

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
  const size_t len = length_dist( gen );
  size_t sum;
  for ( size_t pos = 0; pos < len; pos++ )
  {
    auto &op_dist = operation_dists.at( getIndex( pos, len ) );
    sum = op_dist.back().partial_sum;

    // TODO

  }
  applyPostprocessing( p );
  return p;
}
