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

const int PolynomialMatcher::DEGREE = 1;

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

  while ( factor > 0 )
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
  Settings s;
  Optimizer optimizer( s );
  if ( pol.size() > 1 && pol.at( 1 ) > 0 )
  {
    std::unordered_set<number_t> used_cells;
    number_t largest_used = 0;
    if ( !optimizer.getUsedMemoryCells( p, used_cells, largest_used ) )
    {
      return false;
    }
    number_t saved_arg = std::max( largest_used + 1, (number_t) 2 );
    p.ops.insert( p.ops.begin(),
        Operation( Operation::Type::MOV, Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg ),
            Operand( Operand::Type::MEM_ACCESS_DIRECT, 0 ) ) );
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::LPB, Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg ),
            Operand( Operand::Type::CONSTANT, 1 ) ) );
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, pol.at( 1 ) ) ) );
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, saved_arg ),
            Operand( Operand::Type::CONSTANT, 1 ) ) );
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::LPE, Operand( Operand::Type::CONSTANT, 0 ),
            Operand( Operand::Type::CONSTANT, 1 ) ) );

  }
  else if ( pol.size() > 1 && pol.at( 1 ) < 0 )
  {
    return false;
  }
  if ( pol.size() > 0 && pol.at( 0 ) > 0 )
  {
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::ADD, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, pol.at( 0 ) ) ) );
  }
  else if ( pol.size() > 0 && pol.at( 0 ) < 0 )
  {
    p.ops.insert( p.ops.end(),
        Operation( Operation::Type::SUB, Operand( Operand::Type::MEM_ACCESS_DIRECT, 1 ),
            Operand( Operand::Type::CONSTANT, -pol.at( 0 ) ) ) );
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
