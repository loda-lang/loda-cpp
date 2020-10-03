#include "generator_v2.hpp"

#include "stats.hpp"

GeneratorV2::GeneratorV2( const Settings &settings, int64_t seed )
    :
    Generator( settings, seed )
{
  Stats stats;
  stats.load( "stats" );

  operations.resize( stats.num_operations.size() );
  std::vector<double> probs( stats.num_operations.size() );
  size_t i = 0;
  for ( auto &it : stats.num_operations )
  {
    operations[i] = it.first;
    probs[i] = it.second;
    i++;
  }
  operation_dist = std::discrete_distribution<>( probs.begin(), probs.end() );

  probs.resize( stats.num_programs_per_length.size() );
  for ( i = 0; i < stats.num_programs_per_length.size(); i++ )
  {
    probs[i] = stats.num_programs_per_length[i];
  }
  length_dist = std::discrete_distribution<>( probs.begin(), probs.end() );
}

std::pair<Operation, double> GeneratorV2::generateOperation()
{
  std::pair<Operation, double> next_op;
  next_op.first = operations.at( operation_dist( gen ) );
  next_op.second = (double) (gen() % 100) / 100.0;
  return next_op;
}

Program GeneratorV2::generateProgram()
{
  Program p;
  int64_t length = length_dist( gen );
  generateStateless( p, length );
  auto written_cells = fixCausality( p );
  ensureSourceNotOverwritten( p );
  ensureTargetWritten( p, written_cells );
  ensureMeaningfulLoops( p );
  return p;
}
