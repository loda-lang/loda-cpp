#include "generator_v2.hpp"

#include "stats.hpp"

GeneratorV2::GeneratorV2( const Settings &settings, int64_t seed )
    :
    Generator( settings, seed )
{
  // initialize distributions
  Stats stats;
  stats.load( "stats" );
}

std::pair<Operation, double> GeneratorV2::generateOperation()
{
  std::pair<Operation, double> next_op;
  return next_op;
}

Program GeneratorV2::generateProgram()
{
  Program p;
  generateStateless( p, settings.num_operations );
  auto written_cells = fixCausality( p );
  ensureSourceNotOverwritten( p );
  ensureTargetWritten( p, written_cells );
  ensureMeaningfulLoops( p );
  return p;
}
