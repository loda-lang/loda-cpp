#include "matcher.hpp"

#include "optimizer.hpp"

// --- AbstractMatcher --------------------------------------------------------

template<class T>
void AbstractMatcher<T>::insert( const Sequence &norm_seq, number_t id )
{
  auto reduced = reduce( norm_seq );
  data[id] = reduced.second;
  ids[reduced.first].push_back( id );
}

template<class T>
void AbstractMatcher<T>::remove( const Sequence &norm_seq, number_t id )
{
  auto reduced = reduce( norm_seq );
  ids.remove( reduced.first, id );
  data.erase( id );
}

template<class T>
void AbstractMatcher<T>::match( const Program &p, const Sequence &norm_seq, seq_programs_t &result ) const
{
  auto reduced = reduce( norm_seq );
  auto it = ids.find( reduced.first );
  if ( it != ids.end() )
  {
    for ( auto id : it->second )
    {
      Program copy = p;
      if ( extend( copy, data.at( id ), reduced.second ) )
      {
        result.push_back( std::pair<number_t, Program>( id, copy ) );
      }
    }
  }
}

// --- Direct Matcher ---------------------------------------------------------

std::pair<Sequence, int> DirectMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, int> result;
  result.first = seq;
  result.second = 0;
  return result;
}

bool DirectMatcher::extend( Program &p, int base, int gen ) const
{
  return true;
}

// --- Polynomial Matcher -----------------------------------------------------

const int PolynomialMatcher::DEGREE = 6; // magic number

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

Polynomial PolynomialMatcher::reduce( Sequence &seq, int64_t degree ) const
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

  int64_t min_factor = std::max( (int64_t) 0, factor - 8 ); // magic number
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

std::pair<Sequence, Polynomial> PolynomialMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, Polynomial> result;
  result.first = seq;
  result.second = reduce( result.first, DEGREE );
  return result;
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

bool PolynomialMatcher::extend( Program &p, Polynomial base, Polynomial gen ) const
{
  auto diff = base - gen;
  return addPostPolynomial( p, diff );
}

// --- Delta Matcher ----------------------------------------------------------

const int DeltaMatcher::MAX_DELTA = 5; // magic number

std::pair<Sequence, int> DeltaMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, int> result;
  result.first = seq;
  result.second = 0;
  for ( int i = 0; i < MAX_DELTA; i++ )
  {
    Sequence next;
    next.resize( result.first.size() - 1 );
    bool ok = true;
    for ( size_t j = 0; j < next.size(); j++ )
    {
      if ( result.first[j] < result.first[j + 1] )
      {
        next[j] = result.first[j + 1] - result.first[j];
      }
      else
      {
        ok = false;
        break;
      }
    }
    if ( ok )
    {
      result.first = next;
      result.second++;
    }
    else
    {
      break;
    }
  }
  return result;
}

bool DeltaMatcher::extend( Program &p, int base, int gen ) const
{
  return false;
}
