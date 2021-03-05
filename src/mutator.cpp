#include "mutator.hpp"

Mutator::Mutator( int64_t seed )
{
  gen.seed( seed );
}

void Mutator::mutateConstants( const Program &program, size_t num_results, std::stack<Program> &result )
{
  std::vector<size_t> indices;
  for ( size_t i = 0; i < program.ops.size(); i++ )
  {
    if ( Operation::Metadata::get( program.ops[i].type ).num_operands == 2
        && program.ops[i].source.type == Operand::Type::CONSTANT )
    {
      indices.resize( indices.size() + 1 );
      indices[indices.size() - 1] = i;
    }
  }
  if ( indices.empty() )
  {
    return;
  }
  int64_t var = std::max<int64_t>( 1, num_results / indices.size() );
  for ( size_t i : indices )
  {
    number_t b = program.ops[i].source.value;
    number_t s = b - std::min<number_t>( var / 2, b );
    for ( number_t v = s; v <= s + var; v++ )
    {
      if ( v != b )
      {
        auto p = program;
        p.ops[i].source.value = v;
        result.push( p );
      }
    }
  }
}
