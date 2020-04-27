#include "matcher.hpp"

#include "optimizer.hpp"
#include "reducer.hpp"
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
  if ( !shouldMatchSequence( norm_seq ) )
  {
    return;
  }
  auto reduced = reduce( norm_seq );
  if ( !shouldMatchSequence( reduced.first ) )
  {
    return;
  }
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

template<class T>
bool AbstractMatcher<T>::shouldMatchSequence( const Sequence &seq ) const
{
  if ( backoff && match_attempts.find( seq ) != match_attempts.end() )
  {
    Log::get().debug( "Back off matching of already matched sequence " + seq.to_string() );
    return false;
  }
  match_attempts.insert( seq );
  return true;
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

std::pair<Sequence, line> LinearMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, line> result;
  result.first = seq;
  result.second.offset = Reducer::truncate( result.first );
  result.second.factor = Reducer::shrink( result.first );
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
  result.second.factor = Reducer::shrink( result.first );
  result.second.offset = Reducer::truncate( result.first );
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

std::pair<Sequence, Polynomial> PolynomialMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, Polynomial> result;
  result.first = seq;
  result.second = Reducer::polynomial( result.first, DEGREE );
  return result;
}

bool PolynomialMatcher::extend( Program &p, Polynomial base, Polynomial gen ) const
{
  Settings settings;
  Optimizer optimizer( settings );
  auto diff = base - gen;

  // constant term
  if ( diff.size() > 0 )
  {
    const auto constant = diff[0];
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
  if ( diff.size() > 1 )
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
    for ( number_t exp = 1; exp < diff.size(); exp++ )
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
      const auto factor = diff[exp];
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

// --- Delta Matcher ----------------------------------------------------------

const int DeltaMatcher::MAX_DELTA = 5; // magic number

std::pair<Sequence, delta_t> DeltaMatcher::reduce( const Sequence &seq ) const
{
  std::pair<Sequence, delta_t> result;
  result.first = seq;
  result.second = Reducer::delta( result.first, MAX_DELTA );
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
  auto saved_arg_cell = largest_used + 1;
  auto saved_result_cell = largest_used + 2;
  auto loop_counter_cell = largest_used + 3;
  auto tmp_counter_cell = largest_used + 4;

  Program prefix;
  prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::DIRECT, 0 );
  if ( sum )
  {
    prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::DIRECT, 0 );
    prefix.push_back( Operation::Type::ADD, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  }
  else
  {
    prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 2 );
  }
  prefix.push_back( Operation::Type::LPB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  prefix.push_back( Operation::Type::CLR, Operand::Type::DIRECT, 0, Operand::Type::CONSTANT, largest_used + 1 );
  prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, loop_counter_cell, Operand::Type::CONSTANT, 1 );
  prefix.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, saved_arg_cell );
  if ( sum )
  {
    prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, loop_counter_cell );
  }
  else
  {
    prefix.push_back( Operation::Type::ADD, Operand::Type::DIRECT, 0, Operand::Type::DIRECT, loop_counter_cell );
    prefix.push_back( Operation::Type::SUB, Operand::Type::DIRECT, 0, Operand::Type::CONSTANT, 1 );
  }
  p.ops.insert( p.ops.begin(), prefix.ops.begin(), prefix.ops.end() );

  if ( sum )
  {
    p.push_back( Operation::Type::ADD, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT, 1 );
  }
  else
  {
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::DIRECT,
        loop_counter_cell );
    p.push_back( Operation::Type::LPB, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT, 1 );
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, tmp_counter_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );
  }
  p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );

  if ( sum )
  {
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 1, Operand::Type::DIRECT, saved_result_cell );
  }
  else
  {
    p.push_back( Operation::Type::LPB, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::CONSTANT, 1 );
    p.push_back( Operation::Type::SUB, Operand::Type::DIRECT, saved_result_cell, Operand::Type::DIRECT, 1 );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, saved_arg_cell, Operand::Type::CONSTANT, 0 );
    p.push_back( Operation::Type::LPE, Operand::Type::CONSTANT, 0, Operand::Type::CONSTANT, 0 );
    p.push_back( Operation::Type::MOV, Operand::Type::DIRECT, 1, Operand::Type::DIRECT, saved_result_cell );
  }
  return true;
}

bool extend_delta_it( Program &p, int delta )
{
  while ( delta < 0 )
  {
    if ( !extend_delta( p, false ) )
    {
      return false;
    }
    delta++;
  }
  while ( delta > 0 )
  {
    if ( !extend_delta( p, true ) )
    {
      return false;
    }
    delta--;
  }
  return true;
}

bool DeltaMatcher::extend( Program &p, delta_t base, delta_t gen ) const
{
  if ( base.offset == gen.offset && base.factor == gen.factor )
  {
    return extend_delta_it( p, base.delta - gen.delta );
  }
  else
  {
    if ( !extend_delta_it( p, -gen.delta ) )
    {
      return false;
    }

    // TODO: reuse linear matcher code
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

    if ( !extend_delta_it( p, base.delta ) )
    {
      return false;
    }
  }
  return true;
}
