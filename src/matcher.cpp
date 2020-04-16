#include "matcher.hpp"

#include "optimizer.hpp"
#include "semantics.hpp"

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

// --- Linear Matcher ---------------------------------------------------------

int truncate( Sequence &seq )
{
  int offset = seq.min();
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    seq[i] = seq[i] - offset;
  }
  return offset;
}

int shrink( Sequence &seq )
{
  int factor = -1;
  for ( size_t i = 0; i < seq.size(); i++ )
  {
    if ( seq[i] > 0 )
    {
      if ( factor == -1 )
      {
        factor = seq[i];
      }
      else
      {
        factor = Semantics::gcd( factor, seq[i] );
      }
    }
  }
  if ( factor == -1 )
  {
    factor = 1;
  }
  if ( factor > 1 )
  {
    for ( size_t i = 0; i < seq.size(); i++ )
    {
      seq[i] = seq[i] / factor;
    }
  }
  return factor;
}

std::pair<Sequence, line> LinearMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, line> result;
  result.first = seq;
  result.second.offset = truncate( result.first );
  result.second.factor = shrink( result.first );
  return result;
}

bool LinearMatcher::extend( Program &p, line base, line gen ) const
{
  if ( gen.offset > 0 )
  {
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, gen.offset );
  }
  if ( gen.factor > 1 )
  {
    p.push_back( Operation::Type::DIV, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, gen.factor );
  }
  if ( base.factor > 1 )
  {
    p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, base.factor );
  }
  if ( base.offset > 0 )
  {
    p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, base.offset );
  }
  return true;
}

std::pair<Sequence, line> LinearMatcher2::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, line> result;
  result.first = seq;
  result.second.factor = shrink( result.first );
  result.second.offset = truncate( result.first );
  return result;
}

bool LinearMatcher2::extend( Program &p, line base, line gen ) const
{
  if ( gen.factor > 1 )
  {
    p.push_back( Operation::Type::DIV, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, gen.factor );
  }
  if ( gen.offset > 0 )
  {
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, gen.offset );
  }
  if ( base.offset > 0 )
  {
    p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, base.offset );
  }
  if ( base.factor > 1 )
  {
    p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, base.factor );
  }
  return true;
}

// --- Polynomial Matcher -----------------------------------------------------

const int PolynomialMatcher::DEGREE = 3; // magic number

Sequence subPoly( const Sequence &s, int64_t factor, int64_t exp )
{
  Sequence t;
  t.resize( s.size() );
  for ( size_t x = 0; x < s.size(); x++ )
  {
    t[x] = s[x] - (factor * Semantics::pow( x, exp ));
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
    auto x_exp = Semantics::pow( x, degree );
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
      p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, constant );
    }
    else if ( constant < 0 )
    {
      p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 1, Operand::Type::CONSTANT, -constant );
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
    const number_t term_cell = max_cell + 3;

    // save argument
    p.push_front( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::DIRECT, 0 );

    // polynomial evaluation code
    for ( number_t exp = 1; exp < pol.size(); exp++ )
    {
      // update x^exp
      if ( exp == 1 )
      {
        p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, x_cell, Operand::Type::DIRECT, saved_arg_cell );
      }
      else
      {
        p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, x_cell, Operand::Type::DIRECT, saved_arg_cell );
      }

      // update result
      const auto factor = pol[exp];
      if ( factor > 0 )
      {
        p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, term_cell, Operand::Type::DIRECT, x_cell );
        p.push_back( Operation::Type::MUL, Operand::Type::DIRECT, term_cell, Operand::Type::CONSTANT, factor );
        p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 1, Operand::Type::DIRECT, term_cell );
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
  if ( result.first.size() != seq.size() - MAX_DELTA )
  {
    result.first.resize( seq.size() - MAX_DELTA );
  }
  return result;
}

bool extend_delta( Program &p, const bool sum )
{
  Settings settings;
  Optimizer optimizer( settings );
  std::unordered_set<number_t> used_cells;
  number_t largest_used = 0;
  if ( !optimizer.getUsedMemoryCells( p, used_cells, largest_used ) )
  {
    return false;
  }
  largest_used = std::max( (number_t) 1, largest_used );
  auto loop_counter_cell = largest_used + 1;

  p.push_front( Operation::Type::MOV, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 2 );
  p.push_front( Operation::Type::LPB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );

  p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );

  // TODO
  return false;
}

bool DeltaMatcher::extend( Program &p, int base, int gen ) const
{
  int delta = base - gen;
  while ( delta < 0 )
  {
    if ( !extend_delta( p, true ) )
    {
      return false;
    }
    delta++;
  }
  while ( delta > 0 )
  {
    if ( !extend_delta( p, false ) )
    {
      return false;
    }
    delta--;
  }
  return true;
}
