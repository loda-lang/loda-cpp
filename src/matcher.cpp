#include "matcher.hpp"

#include "optimizer.hpp"

// --- Direct Matcher --------------------------------------------------

void DirectMatcher::insert( const Sequence &norm_seq, number_t id )
{
  ids[norm_seq].push_back( id );
}

void DirectMatcher::remove( const Sequence &norm_seq, number_t id )
{
  ids.remove( norm_seq, id );
}

void DirectMatcher::match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const
{
  auto it = ids.find( norm_seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      result.push_back( std::pair<number_t, Program>( id, p ) );
    }
  }
}

// --- Polynomial Matcher --------------------------------------------------

const int PolynomialMatcher::DEGREE = 3;

Sequence subPoly( const Sequence &s, int64_t factor, int64_t exp )
{
  Sequence t;
  t.resize( s.size() );
  for ( size_t x = 0; x < s.size(); x++ )
  {
    t[x] = s[x] - (factor * pow( x, exp ));
  }
  return t;
}

Polynomial PolynomialMatcher::reduce( Sequence &seq, int64_t degree )
{
  // recursion end
  if ( degree < 0 )
  {
    return Polynomial();
  }

  // calculate maximum factor for current degree
  int64_t max_factor = -1;
  for ( size_t x = 0; x < seq.size(); x++ )
  {
    auto x_exp = pow( x, degree );
    int64_t new_factor = (x_exp == 0) ? -1 : seq[x] / x_exp;
    max_factor = (max_factor == -1) ? new_factor : (new_factor < max_factor ? new_factor : max_factor);
    if ( max_factor == 0 ) break;
  }

  int64_t factor = max_factor;
  Sequence reduced = subPoly( seq, factor, degree );
  auto poly = reduce( reduced, degree - 1 );
  auto cost = reduced.sum();

  int64_t min_factor = std::max( (int64_t) 0, factor - 10 );
  while ( factor > min_factor )
  {
    Sequence reduced_new = subPoly( seq, factor - 1, degree );
    auto poly_new = reduce( reduced_new, degree - 1 );
    auto cost_new = reduced_new.sum();
    if ( cost_new < cost )
    {
      factor--;
      reduced = reduced_new;
      poly = poly_new;
      cost = cost_new;
    }
    else
    {
      break;
    }
  }
  poly.push_back( factor );
  seq = reduced;
  return poly;
}

void PolynomialMatcher::insert( const Sequence &norm_seq, number_t id )
{
  Sequence seq = norm_seq;
  polynoms[id] = reduce( seq, DEGREE );
  ids[seq].push_back( id ); // must be after reduce!
}

void PolynomialMatcher::remove( const Sequence &norm_seq, number_t id )
{
  Sequence seq = norm_seq;
  reduce( seq, DEGREE );
  ids.remove( seq, id );
  polynoms.erase( id );
}

bool addPostPolynomial( Program &p, const Polynomial &pol )
{
  Settings settings;
  Optimizer optimizer( settings );

  // constant term
  if ( pol.size() > 0 )
  {
    const auto constant = pol[0];
    if ( constant > 0 )
    {
      p.ops.insert( p.ops.end(),
          Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
              Operand( Operand::Type::CONSTANT, constant ) ) );
    }
    else if ( constant < 0 )
    {
      p.ops.insert( p.ops.end(),
          Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
              Operand( Operand::Type::CONSTANT, -constant ) ) );
    }
  }

  // non-constant terms
  if ( pol.size() > 1 )
  {
    std::unordered_set<number_t> used_cells;
    number_t max_cell = 0;
    if ( !optimizer.getUsedMemoryCells( p, used_cells, max_cell ) )
    {
      return false;
    }
    max_cell = std::max( max_cell, (number_t) 1 );
    const number_t saved_arg_cell = max_cell + 1;
    const number_t x_cell = max_cell + 2;
    const number_t x_new_cell = max_cell + 3;
    const number_t x_counter_cell = max_cell + 4;
    const number_t factor_cell = max_cell + 5;

    // save argument
    p.ops.insert( p.ops.begin(),
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg_cell ),
            Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ) ) );

    // polynomial evaluation code
    for ( number_t exp = 1; exp < pol.size(); exp++ )
    {
      // update x^exp
      if ( exp == 1 )
      {
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_cell ),
                Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg_cell ) ) );
      }
      else
      {
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_counter_cell ),
                Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg_cell ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_new_cell ),
                Operand( Operand::Type::CONSTANT, 0 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::LPB, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_counter_cell ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_new_cell ),
                Operand( Operand::Type::MEM_ACCESS_DIRECT, x_cell ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_counter_cell ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::LPE, Operand( Operand::Type::CONSTANT, 0 ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, x_cell ),
                Operand( Operand::Type::MEM_ACCESS_DIRECT, x_new_cell ) ) );
      }

      // update result
      const auto factor = pol[exp];
      if ( factor > 0 )
      {
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, factor_cell ),
                Operand( Operand::Type::CONSTANT, factor ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::LPB, Operand( Operand::Type::MEM_ACCESS_DIRECT, factor_cell ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
                Operand( Operand::Type::MEM_ACCESS_DIRECT, x_cell ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, factor_cell ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
        p.ops.insert( p.ops.end(),
            Operation( Operation::Type::LPE, Operand( Operand::Type::CONSTANT, 0 ),
                Operand( Operand::Type::CONSTANT, 1 ) ) );
      }
      else if ( factor < 0 )
      {
        return false;
      }

    }

  }
  return true;
}

void PolynomialMatcher::match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const
{
  Sequence seq = norm_seq;
  auto pol = reduce( seq, DEGREE );
  auto it = ids.find( seq );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      auto diff = polynoms.at( id ) - pol;
      Program copy = p;
      if ( addPostPolynomial( copy, diff ) )
      {
        result.push_back( std::pair<number_t, Program>( id, copy ) );
      }
    }
  }
}
